#include "TaskPoolModule.h"
#include "WeChatOrderTask.h"
#include "Timer.h"
#include "ServerMessageDefine.h"
#include "log4z.h"
#include "ISeverApp.h"
#include "VerifyRequest.h"
#include "AppleVerifyTask.h"
#include "WeChatVerifyTask.h"
#include "DBVerifyTask.h"
#include "ApnsTask.h"
#include "AnyLoginTask.h"
#include "AsyncRequestQuene.h"
#include "VerifyApp.h"
void CTaskPoolModule::init( IServerApp* svrApp )
{
	IGlobalModule::init(svrApp) ;
	m_tTaskPool.init(this,3);
	m_strWechatNotifyUrl = ((CVerifyApp*)svrApp)->getWebchatNotifyUrl();
	m_jsDBCfg = ((CVerifyApp*)svrApp)->getDBCfg();
}

void CTaskPoolModule::onExit()
{
	getPool().closeAll() ;
}

void CTaskPoolModule::update(float fDeta )
{
	IGlobalModule::update(fDeta) ;
	m_tTaskPool.update();
}

bool CTaskPoolModule::onAsyncRequest(uint16_t nRequestType , const Json::Value& jsReqContent, Json::Value& jsResult )
{
	if ( eAsync_Apns != nRequestType )
	{
		return false ;
	}

	auto p = getPool().getReuseTaskObjByID( eTask_Apns );
	CApnsTask* pTest = (CApnsTask*)p.get();
	pTest->setRequest(jsReqContent);
	getPool().postTask(p);
	return true ;
}

bool CTaskPoolModule::onAsyncRequestDelayResp( uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID, uint16_t nTargetID )
{
	if (eAsync_Make_Order == nRequestType)
	{
		onWechatOrder(nReqSerial,jsReqContent,nTargetID );
		return true;
	}
	else if ( eAsync_Verify_Transcation == nRequestType )
	{
		onVerifyMsg(nReqSerial, jsReqContent, nTargetID);
		return true;
	}
	return false;
}

void CTaskPoolModule::testFunc()
{
	printf("task go \n") ;
	uint32_t nCnt = 1 ;
	while (nCnt--)
	{
		auto p = getPool().getReuseTaskObjByID( eTask_Apns );
		CApnsTask* pTest = (CApnsTask*)p.get();
		
		Json::Value jsRequest, target ;
		target[0u] = 12709990 ;
		jsRequest["apnsType"] = 0 ;
		jsRequest["targets"] = target;
		jsRequest["content"] = "test sfhg" ;
		jsRequest["msgID"] = "fs";
		jsRequest["msgdesc"] = "fhsg" ;
		pTest->setRequest(jsRequest);

		getPool().postTask(p);
	}
}

ITask::ITaskPrt CTaskPoolModule::createTask( uint32_t nTaskID )
{
	switch (nTaskID)
	{
	case eTask_WechatOrder:
		{
			std::shared_ptr<CWeChatOrderTask> pTask ( new CWeChatOrderTask(nTaskID)) ;
			pTask->setWechatNotifyUrl(m_strWechatNotifyUrl.c_str());
			return pTask  ;
		}
		break;
	case eTask_WechatVerify:
		{
			LOGFMTE("not use eTask_WechatVerify ");
			//std::shared_ptr<CWechatVerifyTask> pTask ( new CWechatVerifyTask(nTaskID)) ;
			return nullptr  ;
		}
		break;
	case eTask_AppleVerify:
		{
			std::shared_ptr<CAppleVerifyTask> pTask ( new CAppleVerifyTask(nTaskID)) ;
			return pTask  ;
		}
		break;
	case eTask_DBVerify:
		{
			std::shared_ptr<CDBVerfiyTask> pTask ( new CDBVerfiyTask(nTaskID, m_jsDBCfg )) ;
			return pTask  ;
		}
		break;
	case eTask_Apns:
		{
			std::shared_ptr<CApnsTask> pTask ( new CApnsTask(nTaskID)) ;
			return pTask  ;
		}
		break;
	case eTask_AnyLogin:
		{
			std::shared_ptr<AnyLoginTask> pTask(new AnyLoginTask(nTaskID));
			return pTask;
		}
		break;
	default:
		break;
	}
	return nullptr ;
}

// logic  
void CTaskPoolModule::onWechatOrder( uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nTargetID )
{
	auto pTask = getPool().getReuseTaskObjByID(eTask_WechatOrder);
	CWeChatOrderTask* pTaskObj = (CWeChatOrderTask*)pTask.get();
	// set call back 
	if ( pTask->getCallBack() == nullptr )
	{
		pTask->setCallBack([this](ITask::ITaskPrt ptr )
		{
			CWeChatOrderTask* pTask = (CWeChatOrderTask*)ptr.get();
			auto pOrder = pTask->getCurRequest().get(); 

			Json::Value jsResult;
			jsResult["ret"] = pOrder->nRet;
			jsResult["outTradeNo"] = pOrder->cOutTradeNo;
			jsResult["cPrepayId"] = pOrder->cPrepayId;
			jsResult["channel"] = pOrder->nChannel;
			getSvrApp()->responeAsyncRequest(ID_MSG_PORT_DATA, pOrder->nReqSieralNum, pOrder->nTargetID, jsResult);
			LOGFMTI("finish order for sessionid = %d, ret = %d ",pOrder->nTargetID,pOrder->nRet) ;
		}
		) ;
	}

	// set request info 
	std::shared_ptr<stShopItemOrderRequest> pRe = pTaskObj->getCurRequest() ;
	if ( pRe == nullptr )
	{
		pRe = std::shared_ptr<stShopItemOrderRequest>( new stShopItemOrderRequest );
		pTaskObj->setInfo(pRe);
	}
	memset(pRe.get(),0,sizeof(stShopItemOrderRequest)) ;
	sprintf_s(pRe->cShopDesc,50, jsReqContent["shopDesc"].asCString());
	sprintf_s(pRe->cOutTradeNo,32, jsReqContent["outTradeNo"].asCString());
	pRe->nPrize = jsReqContent["price"].asUInt();
	sprintf_s(pRe->cTerminalIp,17, jsReqContent["ip"].asCString());
	pRe->nChannel = jsReqContent["channel"].asUInt();
	pRe->nTargetID =  nTargetID ;
	pRe->nReqSieralNum = nReqSerial ;
	// do the request 
	getPool().postTask(pTask);
	return  ;
}

static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";  
std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len)  
{  
	std::string ret;  
	int i = 0;  
	int j = 0;  
	unsigned char char_array_3[3];  
	unsigned char char_array_4[4];  

	while (in_len--)  
	{  
		char_array_3[i++] = *(bytes_to_encode++);  
		if (i == 3) {  
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;  
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);  
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);  
			char_array_4[3] = char_array_3[2] & 0x3f;  

			for (i = 0; (i <4) ; i++)  
				ret += base64_chars[char_array_4[i]];  
			i = 0;  
		}  
	}  

	if (i)  
	{  
		for (j = i; j < 3; j++)  
			char_array_3[j] = '/0';  

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;  
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);  
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);  
		char_array_4[3] = char_array_3[2] & 0x3f;  

		for (j = 0; (j < i + 1); j++)  
			ret += base64_chars[char_array_4[j]];  

		while ((i++ < 3))  
			ret += '=';  

	}  

	return ret;  

}  

void CTaskPoolModule::onVerifyMsg( uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nTargetID )
{
	auto nShopItemID = jsReqContent["shopItemID"].asUInt();
	auto transcationID = jsReqContent["transcationID"].asCString();
	auto nChannel = jsReqContent["channel"].asUInt();
	auto nPrice = jsReqContent["price"].asUInt();

	IVerifyTask::VERIFY_REQUEST_ptr pRequest ( new stVerifyRequest() );
	pRequest->nChannel = nChannel ;
	pRequest->nPrice = nPrice;
	pRequest->nShopItemID = nShopItemID;
	pRequest->nTargetID = nTargetID;
	pRequest->nReqSieralNum = nReqSerial;

	LOGFMTD("received a transfaction need to verify shop id = %u userUID = %u channel = %d\n", pRequest->nShopItemID, nTargetID, pRequest->nChannel );

	ITask::ITaskPrt pTask = nullptr ;
	if ( pRequest->nChannel == ePay_AppStore )
	{
		memset(pRequest->pBufferVerifyID, 0, sizeof(pRequest->pBufferVerifyID));
		std::string str = base64_encode((unsigned char*)transcationID,strlen(transcationID));
		memcpy(pRequest->pBufferVerifyID,str.c_str(),strlen(str.c_str()));
		pTask = getPool().getReuseTaskObjByID(eTask_AppleVerify) ;
	}
	else
	{
		LOGFMTE("unknown pay channecl = %d, uid = %d",pRequest->nChannel, pRequest->nTargetID ) ;
		Json::Value jsResult;
		jsResult["ret"] = 1;
		getSvrApp()->responeAsyncRequest(ID_MSG_PORT_DATA, nReqSerial, nTargetID, jsResult);
		return ;
	}

	if ( !pTask )
	{
		LOGFMTE("why verify task is null ? ") ;
		Json::Value jsResult;
		jsResult["ret"] = 1;
		getSvrApp()->responeAsyncRequest(ID_MSG_PORT_DATA, nReqSerial, nTargetID, jsResult);
		return ;
	}

	auto pVerifyTask = (IVerifyTask*)pTask.get();
	pVerifyTask->setVerifyRequest(pRequest) ;
	pVerifyTask->setCallBack([this](ITask::ITaskPrt ptr ) 
	{
		auto pAready = (IVerifyTask*)ptr.get();
		auto pResult = pAready->getVerifyResult() ;
		if ( eVerify_Apple_Error == pResult->eResult )
		{
			LOGFMTE("apple verify Error  uid = %u, channel = %u,shopItem id = %u",pResult->nTargetID,pResult->nChannel,pResult->nShopItemID) ;
			// send to client ;
			sendVerifyResult(pResult) ;
			return ;
		}

		LOGFMTI("apple verify success  uid = %u, channel = %u,shopItem id = %u,go on DB verify",pResult->nTargetID,pResult->nChannel,pResult->nShopItemID) ;
		doDBVerify(pResult);
	} ) ;
	getPool().postTask(pTask);
}

void CTaskPoolModule::sendVerifyResult(std::shared_ptr<stVerifyRequest> & pResult )
{
	// verify success , shold save to db ;
	if ( eVerify_Success == pResult->eResult)
	{
		if (ePay_WeChat == pResult->nChannel)
		{
			pResult->nPrice /= 100; // convert to yuan ;
		}

#ifdef _DEBUG
		return;
#endif // _DEBUG
		Json::Value jssql;
		char pBuffer[512] = { 0 };
		sprintf_s(pBuffer, sizeof(pBuffer),"insert into wxrecharge ( userUID,fee,time,tradeOrder, shopItemID ) values ('%u','%u',now(),'%s',%u );", pResult->nTargetID, pResult->nPrice, pResult->pBufferVerifyID,pResult->nShopItemID  );
		jssql["sql"] = pBuffer;
		getSvrApp()->getAsynReqQueue()->pushAsyncRequest( ID_MSG_PORT_RECORDER_DB,pResult->nTargetID, eAsync_DB_Add,jssql);
	}

	// inform data svr ;
	if (pResult->nChannel == ePay_AppStore)  // zhu dong verify, repone by async respone;
	{
		Json::Value jsResult;
		jsResult["ret"] = pResult->eResult;
		getSvrApp()->responeAsyncRequest(ID_MSG_PORT_DATA, pResult->nReqSieralNum, pResult->nTargetID, jsResult);
	}
	else // other reieved other svr inform ;eg: wechat pay  by asnc 
	{
		Json::Value jsResult;
		jsResult["ret"] = pResult->eResult;
		jsResult["targetID"] = pResult->nTargetID;
		jsResult["shopItemID"] = pResult->nShopItemID;
		jsResult["channel"] = pResult->nChannel;
		getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, pResult->nTargetID,eAsync_Recived_Verify_Result, jsResult);
	}
	
	LOGFMTI( "finish verify transfaction shopid = %u ,uid = %d ret = %d", pResult->nShopItemID, pResult->nTargetID, pResult->eResult ) ;
}

void CTaskPoolModule::doDBVerify(uint32_t nUserUID, uint16_t nShopID, uint8_t nChannel,std::string& strTransfcationID, uint32_t nFee )
{
	IVerifyTask::VERIFY_REQUEST_ptr pRequest(new stVerifyRequest());
	pRequest->nTargetID = nUserUID;
	pRequest->nShopItemID = nShopID;
	pRequest->nChannel = nChannel; 
	pRequest->nPrice = nFee;
	memset(pRequest->pBufferVerifyID,0,sizeof(pRequest->pBufferVerifyID));
	memcpy_s(pRequest->pBufferVerifyID, sizeof(pRequest->pBufferVerifyID),strTransfcationID.data(),strTransfcationID.size());
	doDBVerify(pRequest);
}

void CTaskPoolModule::doDBVerify(IVerifyTask::VERIFY_REQUEST_ptr ptr)
{
	auto pDBTask = getPool().getReuseTaskObjByID(eTask_DBVerify);
	auto pDBVerifyTask = (IVerifyTask*)pDBTask.get();

	pDBVerifyTask->setVerifyRequest(ptr);
	pDBVerifyTask->setCallBack([this](ITask::ITaskPrt ptr){ auto pAready = (IVerifyTask*)ptr.get(); sendVerifyResult(pAready->getVerifyResult()); });
	getPool().postTask(pDBTask);
}

ITask::ITaskPrt CTaskPoolModule::getReuseTask(eTask nTask)
{
	return getPool().getReuseTaskObjByID(nTask);
}

void CTaskPoolModule::postTask(ITask::ITaskPrt pTask)
{
	getPool().postTask(pTask);
}