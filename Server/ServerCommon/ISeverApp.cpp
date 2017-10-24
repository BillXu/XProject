//#include <windows.h>
#include "ISeverApp.h"
#include "log4z.h"
#include "MessageDefine.h"
#include "ServerMessageDefine.h"
#include <time.h>
#include <assert.h>
#include <synchapi.h>
#include "IGlobalModule.h"
#include "AutoBuffer.h"
#include "AsyncRequestQuene.h"
#define TIME_WAIT_FOR_RECONNECT 6
bool IServerApp::init( Json::Value& jsSvrCfg )
{
	srand((unsigned int)time(0));
	m_bRunning = true;
	m_nTargetSvrNetworkID = INVALID_CONNECT_ID;
	
	m_pNetWork = new CNetWorkMgr ;
	m_pNetWork->SetupNetwork(1,TIME_HEAT_BEAT);
	m_pNetWork->AddMessageDelegate(this);

	m_pTimerMgr = CTimerManager::getInstance() ;

	m_fReconnectTick = 0 ;

	m_nFrameCnt = 0;
	m_fFrameTicket = 0;
	m_fOutputfpsTickt = 0;

	m_nCurSvrIdx = 0;
	m_nCurSvrPortMaxCnt = 0;

	for ( uint16_t nIdx = eDefMod_None ; nIdx < eDefMod_Max ;  ++nIdx )
	{
		installModule(nIdx);
	}

	auto jsCenter = jsSvrCfg["centerSvr"];
	setConnectServerConfig(jsCenter["ip"].asCString(),jsCenter["port"].asUInt());
	return true ;
}

IServerApp::IServerApp()
{
	m_pTimerMgr = nullptr ;
	m_pNetWork = nullptr ;
	m_eConnectState = CNetWorkMgr::eConnectType_None ;
	m_fReconnectTick = 0 ;
	m_vAllModule.clear() ;
}

IServerApp::~IServerApp()
{
	for ( auto pp : m_vAllModule )
	{
		delete pp.second ;
		pp.second = nullptr ;
	}

	if ( m_pNetWork )
	{
		m_pNetWork->ShutDown() ;
		delete m_pNetWork ;
		m_pNetWork = nullptr ;
	}

}

bool IServerApp::OnMessage( Packet* pMsg )
{
	CHECK_MSG_SIZE(stMsg,pMsg->_len) ;
	stMsg* pmsg = (stMsg*)pMsg->_orgdata ;
	if ( MSG_VERIFY_SERVER == pmsg->usMsgType )
	{
		stMsgVerifyServerRet* pRet = (stMsgVerifyServerRet*)pmsg;
		if (pRet->nRet)
		{
			LOGFMTE("cur svr type is full , can not connect more stop the svr, cur svr will exit");
			m_isSvrFull = true;
			stop();
			return true;
		}
		m_nCurSvrIdx = pRet->uIdx;
		m_nCurSvrPortMaxCnt = pRet->uMaxSvrCount;
		onConnectedToSvr(pRet->isReconnect);
		LOGFMTD("log verify result idx = %u, max Cnt = %u",m_nCurSvrIdx,m_nCurSvrPortMaxCnt );
		return true;
	}

	if ( MSG_SERVER_DISCONNECT == pmsg->usMsgType)
	{
		stMsgServerDisconnect* pRet = (stMsgServerDisconnect*)pmsg;
		onOtherSvrShutDown((eMsgPort)pRet->nDisconnectPort, pRet->nPortIdx, pRet->nMaxPortCnt);
		return true;
	}

	stMsg* pRet = pmsg;
	if ( pRet->usMsgType != MSG_TRANSER_DATA )
	{
		LOGFMTE("why msg type is not transfer data , type = %d",pRet->usMsgType ) ;
		return true;
	}

	stMsgTransferData* pData = (stMsgTransferData*)pRet ;
	stMsg* preal = (stMsg*)( pMsg->_orgdata + sizeof(stMsgTransferData));

	// check async request 
	if ( preal->usMsgType == MSG_ASYNC_REQUEST )
	{
		stMsgAsyncRequest* pRet = (stMsgAsyncRequest*)preal ;
		Json::Value jsReqContent ;
		if ( pRet->nReqContentLen > 0 )
		{
			char* pBuffer = (char*)pRet ;
			pBuffer += sizeof(stMsgAsyncRequest) ;
			Json::Reader jsReader ;
			jsReader.parse(pBuffer,pBuffer + pRet->nReqContentLen,jsReqContent,false);
		}

		Json::Value jsResult ;
		if ( onAsyncRequest(pRet->nReqType,jsReqContent,jsResult) )
		{
			responeAsyncRequest( pData->nSenderPort, pRet->nReqSerailID, pData->nSessionID, jsResult );
			return true;
		}

		if ( onAsyncRequestDelayResp(pRet->nReqType,pRet->nReqSerailID,jsReqContent,pData->nSenderPort,pData->nSessionID,pRet->nTargetID) )
		{
			LOGFMTD( "req delay respone async rqust type = %u, from port = %u",pRet->nReqType,pData->nSenderPort );
			stMsgAsyncRequestRet msgBack;
			msgBack.cSysIdentifer = (eMsgPort)pData->nSenderPort;
			msgBack.nReqSerailID = pRet->nReqSerailID;
			msgBack.nTargetID = pData->nSessionID;
			msgBack.nResultContentLen = 0;
			msgBack.nRet = 1;
			sendMsg(&msgBack, sizeof(msgBack), pData->nSessionID);
			return true;
		}

		LOGFMTE("async request type = %u , not process from port = %u", pRet->nReqType, pData->nSenderPort);
		assert(0 && "must process the req");
		
		stMsgAsyncRequestRet msgBack;
		msgBack.cSysIdentifer = (eMsgPort)pData->nSenderPort;
		msgBack.nReqSerailID = pRet->nReqSerailID;
		msgBack.nTargetID = pData->nSessionID;
		msgBack.nResultContentLen = 0;
		msgBack.nRet = 2;
		sendMsg(&msgBack, sizeof(msgBack), pData->nSessionID);
		return true ;
	}

	if ( preal->usMsgType == MSG_JSON_CONTENT  )
	{
		stMsgJsonContent* pRet = (stMsgJsonContent*)preal ;
		char* pBuffer = (char*)preal ;
		pBuffer += sizeof(stMsgJsonContent);
		//#ifdef __DEBUG
		static char pLog[2048] = { 0 };
		if ( pRet->nJsLen >= 2048)
		{
			LOGFMTE("session id = %u send a invalid len json msg, len = %u",pData->nSessionID,pRet->nJsLen) ;
			return true;
		}
		memset(pLog,0,sizeof(pLog)) ;
		memcpy_s(pLog,sizeof(pLog),pBuffer,pRet->nJsLen);
		//LOGFMTD("session id = %u rec : %s",pData->nSessionID,pLog);
		
		//#endif // __DEBUG

		Json::Reader reader ;
		Json::Value rootValue ;
		auto bRet = reader.parse(pBuffer,pBuffer + pRet->nJsLen,rootValue,false) ;
		if ( !bRet )
		{
			std::string str(pBuffer, pRet->nJsLen);
			LOGFMTE("recieved session id = %u invalid json msg format error : %s",pData->nSessionID,str.c_str() ) ;
			return true ;
		}

		if ( rootValue[JS_KEY_MSG_TYPE].isNull() || rootValue[JS_KEY_MSG_TYPE].isNumeric() == false )
		{
			LOGFMTE("not have msg key type , session id = %u rec : %s",pData->nSessionID,pLog);
			return true ;
		}

		uint16_t nMsgType = rootValue[JS_KEY_MSG_TYPE].asUInt() ;
		if ( onLogicMsg(rootValue,nMsgType,(eMsgPort)pData->nSenderPort,pData->nSessionID,preal->nTargetID) )
		{
			return true ;
		}
		LOGFMTE("unprocessed json from port = %d , session id = %d js : %s",pData->nSenderPort,pData->nSessionID,pLog) ;
		return false ;
	}

	// normal logic msg ;
	if ( onLogicMsg(preal,(eMsgPort)pData->nSenderPort,pData->nSessionID) )
	{
		return true ;
	}

	LOGFMTE("unprocessed msg = %d , from port = %d , session id = %d",preal->usMsgType,pData->nSenderPort,pData->nSessionID) ;
	return false ;
}

void IServerApp::responeAsyncRequest( uint8_t nTargetPort , uint32_t nReqSerialID , uint32_t nTargetID , Json::Value& jsResult, uint32_t nSenderID )
{
	if ( 0 == nSenderID )
	{
		nSenderID = nTargetID;
	}

	stMsgAsyncRequestRet msgBack;
	msgBack.cSysIdentifer = (eMsgPort)nTargetPort;
	msgBack.nReqSerailID = nReqSerialID;
	msgBack.nTargetID = nTargetID;
	msgBack.nResultContentLen = 0;
	msgBack.nRet = 0;
	if (jsResult.isNull() == true)
	{
		sendMsg(&msgBack, sizeof(msgBack), nSenderID );
	}
	else
	{
		Json::StyledWriter jsWrite;
		auto strResult = jsWrite.write(jsResult);
		msgBack.nResultContentLen = strResult.size();
		CAutoBuffer auBuffer(sizeof(msgBack) + msgBack.nResultContentLen);
		auBuffer.addContent(&msgBack, sizeof(msgBack));
		auBuffer.addContent(strResult.c_str(), msgBack.nResultContentLen);
		sendMsg((stMsg*)auBuffer.getBufferPtr(), auBuffer.getContentSize(), nSenderID );
	}
}

void IServerApp::onOtherSvrShutDown(eMsgPort nSvrPort, uint16_t nSvrIdx, uint16_t nSvrMaxCnt)
{
	LOGFMTD("svr port = %u disconnect, idx = %u, max cnt = %u", nSvrPort, nSvrIdx, nSvrMaxCnt);
	for (auto pp : m_vAllModule)
	{
		if (pp.second->onOtherSvrShutDown(nSvrPort, nSvrIdx, nSvrMaxCnt))
		{
			return ;
		}
	}
	return;
}

bool IServerApp::OnLostSever(Packet* pMsg)
{
	m_nTargetSvrNetworkID = INVALID_CONNECT_ID ;
	LOGFMTE("Target server disconnected !") ;
	m_eConnectState = CNetWorkMgr::eConnectType_Disconnectd ;

	m_fReconnectTick = 0 ;

	onOtherSvrShutDown( ID_MSG_PORT_CENTER, 0, 1);
	return false ;
}

bool IServerApp::OnConnectStateChanged( eConnectState eSate, Packet* pMsg)
{
	m_eConnectState = eConnect_Accepted == eSate ? CNetWorkMgr::eConnectType_Connected : CNetWorkMgr::eConnectType_Disconnectd ;
	if ( eConnect_Accepted == eSate )
	{
		m_nTargetSvrNetworkID = pMsg->_connectID ;
		
		stMsgVerifyServer msgVerify;
		msgVerify.nSeverPortType = getTargetSvrPortType();
		msgVerify.nTargetID = 0;
		msgVerify.nSeverPortType = getLocalSvrMsgPortType();
		msgVerify.isReconnect = getCurSvrMaxCnt() > 0;
		if (msgVerify.isReconnect)
		{
			msgVerify.nPreIdx = getCurSvrIdx();
		}
		m_pNetWork->SendMsg((char*)&msgVerify,sizeof(stMsgVerifyServer),pMsg->_connectID) ;
		LOGFMTI("Connected to Target Svr") ;
		return false ;
	}

	m_nTargetSvrNetworkID = INVALID_CONNECT_ID ;
	LOGFMTE("connect target svr failed, %d seconds later reconnect",TIME_WAIT_FOR_RECONNECT) ;
	return false ;
}

uint64_t getCurTimeByMilliSec()
{
	time_t clock;
	struct tm tm;
	SYSTEMTIME wtm;
	GetLocalTime(&wtm);
	tm.tm_year = wtm.wYear - 1900;
	tm.tm_mon = wtm.wMonth - 1;
	tm.tm_mday = wtm.wDay;
	tm.tm_hour = wtm.wHour;
	tm.tm_min = wtm.wMinute;
	tm.tm_sec = wtm.wSecond;
	tm.tm_isdst = -1;
	clock = mktime(&tm);

	uint64_t nMilli = clock * 1000 + wtm.wMilliseconds;
	return nMilli;
}

bool IServerApp::run()
{
	auto nLastMilliSec = getCurTimeByMilliSec();
	while ( m_bRunning )
	{
		auto tNow = getCurTimeByMilliSec();
		float fDelta = float(tNow - nLastMilliSec) / 1000 ;
		nLastMilliSec = tNow ;

		m_pTimerMgr->Update(fDelta);
		update(fDelta*m_pTimerMgr->GetTimeScale());
		if (m_pNetWork)
		{
			m_pNetWork->update(fDelta*m_pTimerMgr->GetTimeScale());
		}
		Sleep(10);
	}

	onExit();
	LOGFMTI("sleep 4k mili seconds");
	Sleep(4000);
	shutDown();
	LOGFMTI("sleep other 4k mili seconds");
	Sleep(4000);
	return true ;
}

void IServerApp::shutDown()
{
	if ( m_pNetWork )
	{
		m_pNetWork->ShutDown() ;
		m_eConnectState = CNetWorkMgr::eConnectType_None ;
	}
}

bool IServerApp::sendMsg( const char* pBuffer , int nLen )
{
	assert(m_pNetWork && "please invoke IServerApp init" );
	if ( isConnected() )
	{
		m_pNetWork->SendMsg(pBuffer,nLen,m_nTargetSvrNetworkID) ;
	}
	else
	{
		LOGFMTE("target is disconnect , can not send msg");
	}
	return isConnected() ;
}

bool IServerApp::sendMsg( stMsg* pBuffer , uint16_t nLen, uint32_t nSenderID )
{
	if ( isConnected() == false )
	{
		LOGFMTE("target svr is not connect , send msg failed") ;
		return false ;
	}

	if (pBuffer->nTargetID == 0)
	{
		LOGFMTW("msg = %u target = 0 attention",pBuffer->usMsgType );
		pBuffer->nTargetID = nSenderID;
	}
	stMsgTransferData msgTransData ;
	msgTransData.nSenderPort = getLocalSvrMsgPortType() ;
	msgTransData.nSessionID = nSenderID;
	int nLne = sizeof(msgTransData) ;
	if ( nLne + nLen >= MAX_MSG_BUFFER_LEN )
	{
		stMsg* pmsg = (stMsg*)pBuffer ;
		LOGFMTE("msg send to session id = %d , is too big , cannot send , msg id = %d ", nSenderID ,pmsg->usMsgType) ;
		return false;
	}
	memcpy_s(m_pSendBuffer ,sizeof(m_pSendBuffer),&msgTransData,nLne);
	memcpy_s(m_pSendBuffer + nLne ,sizeof(m_pSendBuffer) - nLne, (char*)pBuffer,nLen );
	nLne += nLen ;
	sendMsg(m_pSendBuffer,nLne);
	return true ;
}

bool IServerApp::sendMsg( Json::Value& recvValue, uint16_t nMsgID, uint32_t nSenderID, uint32_t nTargetID,uint8_t nTargetPort )
{
	if ( nMsgID )
	{
		recvValue[JS_KEY_MSG_TYPE] = nMsgID;
		//if ( !recvValue[JS_KEY_MSG_TYPE] )
		//{
		//	recvValue[JS_KEY_MSG_TYPE] = nMsgID ;
		//}
		//else
		//{
		//	//LOGFMTE("msg id = %u ,already have this tag uid = %u",nMsgID,recvValue[JS_KEY_MSG_TYPE].asUInt() ) ;
		//}
	}

	Json::StyledWriter writerJs ;
	std::string strContent = writerJs.write(recvValue);
	//LOGFMTD("session id = %u , target port = %u, send : %s",nSessionID,nTargetPort,strContent.c_str());
	stMsgJsonContent msg ;
	msg.cSysIdentifer = nTargetPort ;
	msg.nJsLen = strContent.size() ;
	msg.nTargetID = nTargetID;
	if ( nTargetID == 0 )
	{
		LOGFMTW("json msg = %u target = 0 attention", nMsgID );
		msg.nTargetID = nSenderID;
	}
	CAutoBuffer bufferTemp(sizeof(msg) + msg.nJsLen);
	bufferTemp.addContent(&msg,sizeof(msg)) ;
	bufferTemp.addContent(strContent.c_str(),msg.nJsLen) ;
	//LOGFMTD("session id = %u , target port = %u, len = %u send : %s",nSessionID,nTargetPort,bufferTemp.getContentSize(),strContent.c_str());
	return sendMsg((stMsg*)bufferTemp.getBufferPtr(),bufferTemp.getContentSize(), nSenderID) ;
}

void IServerApp::stop()
{
	m_bRunning = false ;
}

bool IServerApp::onLogicMsg(stMsg* prealMsg , eMsgPort eSenderPort , uint32_t nSenderID )
{
	for ( auto pp : m_vAllModule )
	{
		if ( pp.second->onMsg(prealMsg,eSenderPort, nSenderID) )
		{
			return true ;
		}
	}
	return false ;
}

bool IServerApp::onLogicMsg( Json::Value& recvValue , uint16_t nmsgType, eMsgPort eSenderPort , uint32_t nSenderID, uint32_t nTargetID )
{
	for ( auto pp : m_vAllModule )
	{
		if ( pp.second->onMsg(recvValue,nmsgType,eSenderPort, nSenderID,nTargetID) )
		{
			return true ;
		}
	}
	return false ;
}

bool IServerApp::onAsyncRequest(uint16_t nRequestType , const Json::Value& jsReqContent, Json::Value& jsResult )
{
	for ( auto pp : m_vAllModule )
	{
		if ( pp.second->onAsyncRequest(nRequestType,jsReqContent,jsResult) )
		{
			return true ;
		}
	}
	return false ;
}

bool IServerApp::onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID, uint16_t nTargetID)
{
	for (auto pp : m_vAllModule)
	{
		if (pp.second->onAsyncRequestDelayResp(nRequestType, nReqSerial, jsReqContent, nSenderPort,nSenderID,nTargetID) )
		{
			return true;
		}
	}
	return false;
}

void IServerApp::update(float fDeta )
{
	if ( m_eConnectState == CNetWorkMgr::eConnectType_Disconnectd && false == m_isSvrFull )
	{
		m_fReconnectTick += fDeta ;
		if ( m_fReconnectTick >= TIME_WAIT_FOR_RECONNECT )
		{
			LOGFMTI("Reconnecting....");
			doConnectToTargetSvr() ;
			m_fReconnectTick = 0 ;
		}
	}

	// moudle update ;
	for ( auto pp : m_vAllModule )
	{
		pp.second->update(fDeta);
	}

	// caculate fps 
#ifdef _DEBUG
	return;
#endif // _DEBUG
	++m_nFrameCnt;
	m_fFrameTicket += fDeta;
	m_fOutputfpsTickt += fDeta;
	if (m_fFrameTicket >= 1.0f)
	{
		if ( m_fOutputfpsTickt > 50 )
		{
			LOGFMTD("FPS : %u\n", m_nFrameCnt);
			m_fOutputfpsTickt = 0;
		}
		else
		{
			printf("FPS : %u\n", m_nFrameCnt);
		}
		
		m_nFrameCnt = 0;
		m_fFrameTicket -= 1.0;
	}
}

uint16_t IServerApp::getTargetSvrPortType()
{
	return ID_MSG_PORT_CENTER ;
}

bool IServerApp::isConnected()
{
	return m_eConnectState == CNetWorkMgr::eConnectType_Connected;
}

void IServerApp::setConnectServerConfig( const char* pCenterSvrIP, uint16_t nCenterSvrPort)
{
	m_strCenterSvrIP = pCenterSvrIP;
	m_nCenterSvrPort = nCenterSvrPort;

	if ( m_eConnectState != CNetWorkMgr::eConnectType_Connected && CNetWorkMgr::eConnectType_Connecting != m_eConnectState )
	{
		doConnectToTargetSvr();
	}
}

void IServerApp::doConnectToTargetSvr()
{
	if ( m_eConnectState == CNetWorkMgr::eConnectType_Connecting || isConnected() )
	{
		return ;
	}

	assert(m_pNetWork && "IServer init not invoke" ) ;
	assert(m_nCenterSvrPort && "please set connect config" ) ;
	m_pNetWork->ConnectToServer(m_strCenterSvrIP.c_str(), m_nCenterSvrPort,"") ;
	m_eConnectState = CNetWorkMgr::eConnectType_Connecting ;
	LOGFMTI("connecting to target svr ip = %s", m_strCenterSvrIP.c_str());
}

void IServerApp::onExit()
{
	for ( auto pp : m_vAllModule )
	{
		pp.second->onExit();
	}
	getNetwork()->RemoveAllDelegate();
}

void IServerApp::onConnectedToSvr( bool isReconnectMode )
{
	for ( auto pp : m_vAllModule )
	{
		pp.second->onConnectedSvr(isReconnectMode);
	}
}

bool IServerApp::registerModule(IGlobalModule* pModule,uint16_t eModuleType)
{
	//assert(pModule && "this module is null" );
	if ( pModule == nullptr )
	{
		return false;
	}

	if ( eModuleType == IGlobalModule::INVALID_MODULE_TYPE )
	{
		assert(0&&"please set the module type ");
		return false ;
	}

	pModule->setModuleType(eModuleType);
	auto pp = getModuleByType(eModuleType) ;
	assert(pp == nullptr && "already have this module" );
	if ( pp )
	{
		return false ;
	}

	m_vAllModule[pModule->getModuleType()] = pModule ;
	pModule->init(this);
	return true ;
}

bool IServerApp::installModule( uint16_t nModuleType )
{
	auto pAsy = createModule(nModuleType) ;
	return registerModule(pAsy,nModuleType) ;
}

IGlobalModule* IServerApp::getModuleByType(uint16_t nType )
{
	auto pp = m_vAllModule.find(nType) ;
	if ( pp != m_vAllModule.end() )
	{
		return pp->second ;
	}
	return nullptr;
}

IGlobalModule* IServerApp::createModule( uint16_t eModuleType )
{
	IGlobalModule* pModule = nullptr ;
	switch (eModuleType)
	{
	case eDefMod_AsyncRequestQueu:
		{
			pModule = new CAsyncRequestQuene ;
		}
		break;
	default:
		break;
	}

	return pModule ;
}

CAsyncRequestQuene* IServerApp::getAsynReqQueue()
{
	return (CAsyncRequestQuene*)getModuleByType(eDefMod_AsyncRequestQueu);
}
