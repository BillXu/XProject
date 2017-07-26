#include "GateServer.h"
#include "CommonDefine.h"
#include "ServerMessageDefine.h"
#include "log4z.h"
#include "GateClient.h"
#include <assert.h>
CGateServer* CGateServer::s_GateServer = NULL ;
CGateServer* CGateServer::SharedGateServer()
{
	return s_GateServer ;
}

CGateServer::CGateServer()
{
	m_pNetWorkForClients = NULL ;
	m_pGateManager = NULL ;
	m_nCurMaxSessionID = 0 ;
	if ( s_GateServer )
	{
		assert(0&&"only once should");
	}
}

CGateServer::~CGateServer()
{
	if ( m_pNetWorkForClients )
	{
		m_pNetWorkForClients->ShutDown() ;
	}
	delete m_pNetWorkForClients ;
	delete m_pGateManager ;
	s_GateServer = NULL ;
}

bool CGateServer::init(Json::Value& jsSvrCfg)
{
	IServerApp::init(jsSvrCfg);
	if ( s_GateServer )
	{
		assert(0&&"only once should");
	}
	s_GateServer = this ;
	// client player mgr ;
	m_pGateManager = new CGateClientMgr ;
	
	if ( jsSvrCfg["gatePort"].isNull() )
	{
		LOGFMTE("log gate port is null");
		return false ;
	}
	m_nGatePort = jsSvrCfg["gatePort"].asUInt();
	return true ;
}

void CGateServer::update(float fDeta )
{
	IServerApp::update(fDeta);  
	if ( m_pNetWorkForClients )
	{
		m_pNetWorkForClients->RecieveMsg() ;
	}
}

void CGateServer::onExit()
{
	m_pGateManager->closeAllClient();
	if (m_pNetWorkForClients)
	{
		m_pNetWorkForClients->ShutDown();
	}
}

void CGateServer::onConnectedToSvr( bool isReconnectMode )
{
	// start gate svr for client to connected 
	if (nullptr == m_pNetWorkForClients)
	{
		m_pNetWorkForClients = new CServerNetwork;
		m_pNetWorkForClients->StartupNetwork(m_nGatePort, 5000, "");
		m_pNetWorkForClients->AddDelegate(m_pGateManager);
	}

	LOGFMTI("setup network for clients to client ok ");
	LOGFMTI("Gate Server Start ok idx = %d ", getCurSvrIdx());
}

bool CGateServer::onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)
{
	if ( eAsync_InformGate_PlayerLogout == nRequestType)
	{
		auto nSessionID = jsReqContent["sessionID"].asUInt();
		m_pGateManager->onPlayerLogout(nSessionID);
		return true;
	}
	return false;
}

void CGateServer::sendMsgToClient(const char* pData , int nLength , CONNECT_ID nSendToOrExcpet ,bool bBroadcast )
{
	if ( m_pNetWorkForClients )
	{
		m_pNetWorkForClients->SendMsg(pData,nLength,nSendToOrExcpet,bBroadcast) ;
	}
}

bool CGateServer::OnMessage( Packet* pPacket )
{
	stMsg* pMsg = (stMsg*)pPacket->_orgdata ;
	if ( MSG_TRANSER_DATA == pMsg->usMsgType )
	{
		stMsgTransferData* pData = (stMsgTransferData*)pMsg;
		stMsg* prealMsg = (stMsg*)(pPacket->_orgdata + sizeof(stMsgTransferData));
		if ( ID_MSG_PORT_CLIENT == prealMsg->cSysIdentifer )
		{
			auto pGate = m_pGateManager->getGateClientBySessionID(prealMsg->nTargetID);
			if (pGate == nullptr)
			{
				LOGFMTE("why send msg to nullptr gate msg = %u, target = %u, senderID = %u", prealMsg->usMsgType, prealMsg->nTargetID, pData->nSessionID );
				return true;
			}

			if (pGate->isWaitingReconnect())
			{
				LOGFMTE("gate is waiting reconnect , should not send msg to it , msg = %u, sender port = %u , nSendID = %u, targetID = %u", prealMsg->usMsgType, pData->nSenderPort, pData->nSessionID, prealMsg->nTargetID );
				return true;
			}
			sendMsgToClient((const char*)prealMsg, pPacket->_len - sizeof(stMsgTransferData) , pGate->getNetworkID());
			return true;
		}
	}
	return IServerApp::OnMessage(pPacket);
}

bool CGateServer::onLogicMsg(stMsg* prealMsg, eMsgPort eSenderPort, uint32_t nSenderID)
{
	if ( IServerApp::onLogicMsg( prealMsg, eSenderPort, nSenderID ) )
	{
		return true;
	}
  
	if (ID_MSG_PORT_GATE != prealMsg->cSysIdentifer)
	{
		return false;
	}

	if ( MSG_TELL_GATE_PLAYER_OTHER_LOGIN == prealMsg->usMsgType)
	{
		auto pGateClient = getClientMgr()->getGateClientBySessionID(prealMsg->nTargetID);
		if ( pGateClient == nullptr)
		{
			LOGFMTW("old client is null , so need not delete uid = %u",nSenderID );
			return true;
		}
		pGateClient->bindUID(0);
		pGateClient->delayClose();
		LOGFMTD("old client will close , other logined , uid = %u",nSenderID );
		return true;
	}
	
	return false;
}


uint32_t CGateServer::generateSessionID()
{
	if ( 0 == m_nCurMaxSessionID)
	{
		m_nCurMaxSessionID = getCurSvrIdx() + getCurSvrMaxCnt();
		return m_nCurMaxSessionID;
	}
	return (m_nCurMaxSessionID += getCurSvrMaxCnt());
}

