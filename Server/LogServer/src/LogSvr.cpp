#include "LogSvrApp.h"
#include "DBRequest.h"
#include "ServerMessageDefine.h"
#include "CommonDefine.h"
#include "log4z.h"
#include "ConfigDefine.h"
#include "AutoBuffer.h"
bool CLogSvrApp::init(Json::Value& jsSvrCfg)
{
	IServerApp::init(jsSvrCfg);
	// set up data base thread 
	auto jsDB = jsSvrCfg["gameDB"];
	if (jsDB.isNull())
	{
		LOGFMTE("data base config is null, cant not start server");
		return false;
	}

	// dbManager ;
	m_tDBRW.init(jsDB["ip"].asCString(), jsDB["account"].asCString(), jsDB["pwd"].asCString(), jsDB["name"].asCString());

	LOGFMTI("DBServer Start!");
	return true;
}

void CLogSvrApp::update(float fDeta)
{
	IServerApp::update(fDeta);
	// process DB Result ;
	m_tDBRW.update();
}

void CLogSvrApp::onExit()
{
	m_tDBRW.closeAll();
	LOGFMTI("DBServer ShutDown!");
}

bool CLogSvrApp::onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID, uint16_t nTargetID)
{
	if (IServerApp::onAsyncRequestDelayResp(nRequestType, nReqSerial, jsReqContent, nSenderPort, nSenderID, nTargetID))
	{
		return true;
	}

	if (jsReqContent["sql"].isNull())
	{
		LOGFMTE("sql is null reqType = %u, from srcPort = %u, serialNum = %u", nRequestType, nSenderPort, nReqSerial);
		return false;
	}
#ifdef _DEBUG
	std::string strSql = jsReqContent["sql"].asString();
	std::transform(strSql.begin(), strSql.end(), strSql.begin(), ::tolower);
	eDBRequestType nDBTypeGuess = eDBRequestType::eRequestType_Max;
	// check selectType 
	if (strSql.find("select") < 10 || strSql.find("call") < 10)
	{
		nDBTypeGuess = eDBRequestType::eRequestType_Select;
	}
	else if (strSql.find("update") < 10)
	{
		nDBTypeGuess = eDBRequestType::eRequestType_Update;
	}
	else if (strSql.find("delete") < 10)
	{
		nDBTypeGuess = eDBRequestType::eRequestType_Delete;
	}
	else if (strSql.find("insert") < 10)
	{
		nDBTypeGuess = eDBRequestType::eRequestType_Add;
	}
	else
	{
		LOGFMTE("can not tell from sql , what req type : sql = %s", strSql.c_str());
	}
#endif

	eDBRequestType nDBType = eDBRequestType::eRequestType_Select;
	switch (nRequestType)
	{
	case eAsync_DB_Select:
	{
		nDBType = eDBRequestType::eRequestType_Select;
	}
	break;
	case eAsync_DB_Update:
	{
		nDBType = eDBRequestType::eRequestType_Update;
	}
	break;
	case eAsync_DB_Add:
	{
		nDBType = eDBRequestType::eRequestType_Add;
	}
	break;
	case eAsync_DB_Delete:
	{
		nDBType = eDBRequestType::eRequestType_Delete;
	}
	break;
	default:
		LOGFMTD("unknown reqType = %u, from srcPort = %u, serialNum = %u", nRequestType, nSenderPort, nReqSerial);
		return false;
	}

#ifdef _DEBUG
	if (nDBType != nDBTypeGuess && nDBTypeGuess != eDBRequestType::eRequestType_Max)
	{
		LOGFMTE("intent type = %u not the same with guessType = %u sql = %s", nDBType, nDBTypeGuess, strSql.c_str());
	}
#endif


	auto pRequest = getDBModule()->getReserveReqObj();
	pRequest->nRequestUID = nReqSerial;
	pRequest->pUserData = nullptr;
	pRequest->eType = nDBType;
	pRequest->nSqlBufferLen = 0;
	pRequest->nSqlBufferLen = sprintf_s(pRequest->pSqlBuffer, sizeof(pRequest->pSqlBuffer), "%s", jsReqContent["sql"].asCString());
	LOGFMTD("receive async db reqType = %d , srcPort = %u , serial number = %u", nRequestType, nSenderPort, nReqSerial);

	getDBModule()->postDBRequest(pRequest, [nSenderPort, nSenderID, nTargetID, this](stDBRequest::s_ptr pReq, std::shared_ptr<stDBResult> pResult)
	{
		// make result to js value ;
		Json::Value jsResult;
		jsResult["afctRow"] = pResult->nAffectRow;
		Json::Value jsData;
		for (uint16_t nIdx = 0; nIdx < pResult->nAffectRow && pResult->vResultRows.size() > nIdx; ++nIdx)
		{
			CMysqlRow& pRow = *pResult->vResultRows[nIdx];
			Json::Value jsRow;
			pRow.toJsValue(jsRow);
			jsData[jsData.size()] = jsRow;
		}
		jsResult["data"] = jsData;

		// send back ;
		stMsgAsyncRequestRet msgBack;
		msgBack.cSysIdentifer = (eMsgPort)nSenderPort;
		msgBack.nReqSerailID = pReq->nRequestUID;
		msgBack.nTargetID = nSenderID;
		msgBack.nResultContentLen = 0;
		if (jsResult.isNull() == true)
		{
			sendMsg(&msgBack, sizeof(msgBack), nTargetID);
		}
		else
		{
			Json::StyledWriter jsWrite;
			auto strResult = jsWrite.write(jsResult);
			msgBack.nResultContentLen = strResult.size();
			CAutoBuffer auBuffer(sizeof(msgBack) + msgBack.nResultContentLen);
			auBuffer.addContent(&msgBack, sizeof(msgBack));
			auBuffer.addContent(strResult.c_str(), msgBack.nResultContentLen);
			sendMsg((stMsg*)auBuffer.getBufferPtr(), auBuffer.getContentSize(), nTargetID);
		}
		LOGFMTD("processed async req from = %u , serailNum = %u", nSenderPort, nSenderID);
		return;
	});
	return true;
}