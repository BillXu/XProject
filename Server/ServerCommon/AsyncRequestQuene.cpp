#include "AsyncRequestQuene.h"
#include <cassert>
#include <ctime>
#include "AutoBuffer.h"
#include "ISeverApp.h"
#include "log4z.h"
#include "catch_dump_file.h"
#define  TIME_CHECK_REQ_STATE 20
void CAsyncRequestQuene::init( IServerApp* svrApp )
{
	IGlobalModule::init(svrApp) ;
	m_nReqSerailNum = 0 ;

	m_tCheckReqStateTimer.setCallBack(timer_bind_obj_func(this,CAsyncRequestQuene::timerCheckReqState));
	m_tCheckReqStateTimer.setIsAutoRepeat(true) ;
	m_tCheckReqStateTimer.setInterval(TIME_CHECK_REQ_STATE) ;
	m_tCheckReqStateTimer.start() ;
}

bool tempFunc( CAsyncRequestQuene::stAsyncRequest* pReq,Json::Value& jsResultContent, Json::Value& jsUserData )
{
	//LOGFMTD("tempFunc");
	__try
	{
		pReq->lpCallBack(pReq->nReqType, jsResultContent, jsUserData );
		return true;
	}
	__except (CatchDumpFile::CDumpCatch::UnhandledExceptionFilterEx(GetExceptionInformation()))
	{
		//LOGFMTI("catertsf ----- tempFunc");
		return false;
	}
	return true;
}

bool CAsyncRequestQuene::onMsg(stMsg* prealMsg , eMsgPort eSenderPort , uint32_t nSessionID)
{
	if ( prealMsg->usMsgType != MSG_ASYNC_REQUEST_RESULT )
	{
		return false ;
	}

	stMsgAsyncRequestRet* pRet = (stMsgAsyncRequestRet*)prealMsg ;

	Json::Value jsResultContent ;
	if ( pRet->nResultContentLen > 0 )
	{
		char* pBuffer = (char*)prealMsg;
		pBuffer += sizeof(stMsgAsyncRequestRet);
		Json::Reader jsReader ;
		jsReader.parse(pBuffer,pBuffer + pRet->nResultContentLen,jsResultContent) ;
		//LOGFMTD("as str : %s",pBuffer);
	}

	auto pReqIter = m_mapRunningRequest.find(pRet->nReqSerailID) ;
	if ( pReqIter != m_mapRunningRequest.end() )
	{
		auto pReq = pReqIter->second ;
		if (pRet->nRet == 1) // delay respone 
		{
			pReq->tLastSend -= ( TIME_CHECK_REQ_STATE * 3 );
			LOGFMTD("this request deay respone do not try again during some seconds");
			return true;
		}

		if ( pReq->lpCallBack )
		{
			//pReq->lpCallBack(pReq->nReqType, jsResultContent, pReq->jsUserData);
			auto bRet = tempFunc(pReq, jsResultContent, pReq->jsUserData );
			if (!bRet)
			{
				LOGFMTE("do have a exption for this request type = %u",pReq->nReqType );
				canncelAsyncRequest(pRet->nReqSerailID);
				return true;
			}
		}
		else
		{
			//LOGFMTD("type = %u , serial num = %u , request func is null" , pReq->nReqType,pReq->nReqSerialNum) ;
		}
	}
	else
	{
		LOGFMTD("serial num = %u , already canncel" , pRet->nReqSerailID) ;
		return true;
	}


	// end the req ;
	canncelAsyncRequest(pRet->nReqSerailID);
	return true ;
}

CAsyncRequestQuene::~CAsyncRequestQuene()
{
	for ( auto pp : m_vReserverReqObject )
	{
		delete pp ;
		pp = nullptr ;
	}

	for (  auto iter : m_mapRunningRequest )
	{
		delete iter.second ;
		iter.second ;
	}

	m_vReserverReqObject.clear() ;
	m_mapRunningRequest.clear() ;
}

uint32_t CAsyncRequestQuene::pushAsyncRequest(uint8_t nTargetPortID, uint32_t nTargetID, uint32_t nSenderID, uint16_t nReqType,Json::Value& reqContent, async_req_call_back_func lpCallBack,Json::Value& jsUserData, uint32_t nRequestUID )
{
	if (nRequestUID > 0)
	{
		auto pReq = getAsynRequestByRequestUID(nRequestUID);
		if ( pReq )
		{
			LOGFMTW("already sended this rquest type = %u, port = %u target id = %u",nReqType,nTargetPortID,nTargetID);
			return pReq->nReqSerialNum;
		}
	}

	auto pReq = getReuseAsyncReqObject() ;
	pReq->jsReqContent = reqContent ;
	pReq->jsUserData = jsUserData ;
	pReq->lpCallBack = lpCallBack ;
	pReq->nReqSerialNum = ++m_nReqSerailNum ;
	pReq->nReqType = nReqType ;
	pReq->nSendTimes = 0 ;
	pReq->nTargetPortID = nTargetPortID ;
	pReq->tLastSend = 0 ;
	pReq->nRequestUID = nRequestUID;
	pReq->nTargetID = nTargetID;
	pReq->nSenderID = nSenderID;
	m_mapRunningRequest[pReq->nReqSerialNum] = pReq ;
	sendAsyncRequest(pReq) ;
	return pReq->nReqSerialNum ;
}

uint32_t CAsyncRequestQuene::pushAsyncRequest(uint8_t nTargetPortID, uint32_t nTargetID, uint32_t nSenderID, uint16_t nReqType,Json::Value& reqContent, async_req_call_back_func lpCallBack, uint32_t nRequestUID)
{
	Json::Value jsValue ;
	return pushAsyncRequest(nTargetPortID, nTargetID, nSenderID,nReqType,reqContent,lpCallBack,jsValue,nRequestUID);
}

uint32_t CAsyncRequestQuene::pushAsyncRequest(uint8_t nTargetPortID, uint32_t nTargetID, uint32_t nSenderID, uint16_t nReqType,Json::Value& reqContent, uint32_t nRequestUID)
{
	return pushAsyncRequest(nTargetPortID, nTargetID, nSenderID, nReqType,reqContent,nullptr, nRequestUID);
}

bool CAsyncRequestQuene::canncelAsyncRequest( uint32_t nReqSerialNum )
{
	auto iter = m_mapRunningRequest.find(nReqSerialNum) ;
	if ( iter != m_mapRunningRequest.end() )
	{
		m_vReserverReqObject.push_back(iter->second) ;
		m_mapRunningRequest.erase(iter) ;
		return true ;
	}
	assert( 0 && "request not runing , how can cannecl");
	return false ;
}

void CAsyncRequestQuene::sendAsyncRequest(stAsyncRequest* pReq )
{
	++pReq->nSendTimes ;
	pReq->tLastSend = time(nullptr) ;
	stMsgAsyncRequest msgReq ;
	msgReq.nTargetID = pReq->nTargetID;
	msgReq.cSysIdentifer = pReq->nTargetPortID ;
	msgReq.nReqSerailID = pReq->nReqSerialNum ;
	msgReq.nReqType = pReq->nReqType ;
	
	Json::StyledWriter jsWriter ;
	auto str = jsWriter.write(pReq->jsReqContent) ;
	msgReq.nReqContentLen = (uint16_t)str.size() ;
	
	CAutoBuffer auBuffer(sizeof(msgReq) + msgReq.nReqType);
	auBuffer.addContent(&msgReq,sizeof(msgReq)) ;
	auBuffer.addContent(str.c_str(),msgReq.nReqContentLen) ;
	getSvrApp()->sendMsg((stMsg*)auBuffer.getBufferPtr(), auBuffer.getContentSize(), pReq->nSenderID);
}

CAsyncRequestQuene::stAsyncRequest* CAsyncRequestQuene::getAsynRequestByRequestUID(uint32_t nRequestUID)
{
	for (auto& refPair : m_mapRunningRequest)
	{
		if (refPair.second->nRequestUID && refPair.second->nRequestUID == nRequestUID)
		{
			return refPair.second;
		}
	}
	return nullptr;
}

CAsyncRequestQuene::stAsyncRequest* CAsyncRequestQuene::getReuseAsyncReqObject()
{
	stAsyncRequest* pReq = nullptr ;
	if ( m_vReserverReqObject.empty() )
	{
		pReq = new stAsyncRequest ;
	}
	else
	{
		pReq = m_vReserverReqObject.front() ;
		m_vReserverReqObject.pop_front() ;
	}
	return pReq ;
}

void CAsyncRequestQuene::timerCheckReqState(CTimer* pTimer, float fTick )
{
	if (m_mapRunningRequest.empty() )
	{
		return ;
	}

	time_t tNow = time(nullptr) ;
	std::vector<uint32_t> vCanncelReq;
	for ( auto pairReq : m_mapRunningRequest )
	{
		auto pReq = pairReq.second ;
		if (pReq->nSendTimes > 10 )
		{
			// tell callBack , not process ;
			if (pReq->lpCallBack)
			{
				Json::Value jsNullResult;
				auto bRet = tempFunc(pReq, jsNullResult, pReq->jsUserData);
				if (!bRet)
				{
					LOGFMTE("do have a exption for this request type = %u", pReq->nReqType);
				}
			}

			vCanncelReq.push_back(pReq->nReqSerialNum);
			continue;
		}

		if ( pReq->tLastSend + TIME_CHECK_REQ_STATE <= tNow )
		{
			sendAsyncRequest(pReq) ;
		}

		if ( pReq->nSendTimes > 1 )
		{
			Json::StyledWriter jsWrite ;
			auto str = jsWrite.write(pReq->jsReqContent);
			LOGFMTE("req type = %u , target port = %u  str = %s, tried too many times = %u , why ",pReq->nReqType,pReq->nTargetPortID,str.c_str(),pReq->nSendTimes) ;
		}
	}

	// do canncel send too many times not respone ;
	for (auto& ref : vCanncelReq)
	{
		canncelAsyncRequest(ref);
	}
	vCanncelReq.clear();
}