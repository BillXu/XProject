#include "ClientManager.h"
#include "GateClient.h"
#include "MessageDefine.h"
#include "log4z.h"
#include "CommonDefine.h"
#include "ServerNetwork.h"
#include "GateServer.h"
#include "ServerMessageDefine.h"
#include "ServerNetwork.h"
#include <time.h>
#define MAX_INCOME_PLAYER 2000
CGateClientMgr::CGateClientMgr()
{
	m_vNetWorkIDGateClientIdx.clear();
	m_vSessionGateClient.clear() ;
	m_vGateClientReserver.clear();
}

CGateClientMgr::~CGateClientMgr()
{
	MAP_SESSIONID_GATE_CLIENT::iterator iterS = m_vSessionGateClient.begin();
	for ( ; iterS != m_vSessionGateClient.end(); ++iterS )
	{
		delete  iterS->second ;
	}
	m_vSessionGateClient.clear() ;

	// just clear ; object deleted in session Gate ;
	m_vNetWorkIDGateClientIdx.clear() ;

	SET_GATE_CLIENT::iterator iter = m_vGateClientReserver.begin() ;
	for ( ; iter != m_vGateClientReserver.end(); ++iter )
	{
		delete *iter ;
	}
	m_vGateClientReserver.clear() ;
}

bool CGateClientMgr::OnMessage( Packet* pData )
{
	// verify identify 
	stMsg* pMsg = (stMsg*)pData->_orgdata ;
	CHECK_MSG_SIZE(stMsg,pData->_len);
	if ( MSG_VERIFY_CLIENT == pMsg->usMsgType )
	{
		auto pGate = GetGateClientByNetWorkID(pData->_connectID);
		if (pGate == nullptr)
		{
			LOGFMTE("you do not connect how to send verify msg");
			return true;
		}
		pGate->doVerifyed();
		return true;
	}

	// client reconnect ;
	if ( MSG_RECONNECT == pMsg->usMsgType )
	{
		stMsgReconnect* pRet = (stMsgReconnect*)pMsg ;
		CHECK_MSG_SIZE(stMsgReconnect,pData->_len);
		auto pBeConnectGate = getGateClientBySessionID(pRet->nSessionID);
		bool bReconnectOk = pBeConnectGate != NULL && pBeConnectGate->getBindUID() > 0 ;
		stGateClient* pCurGate = GetGateClientByNetWorkID(pData->_connectID);
		if (pCurGate == nullptr)
		{
			LOGFMTE("do reconnect why cur player is nullptr ?");
			bReconnectOk = false;
		}

		if ( bReconnectOk )
		{
			// remove origin 
			removeActiveClientGate(pCurGate);
			removeActiveClientGate(pBeConnectGate);
			pCurGate->onReconnected(pBeConnectGate);
			addClientGate(pCurGate);

			pBeConnectGate->reset();
			addToResever(pBeConnectGate );

			LOGFMTI("MSG¡¡reconnected ! session id = %d", pRet->nSessionID);
			stMsgClientConnectStateChanged msgRet;
			msgRet.nCurState = 0;
			msgRet.nTargetID = pCurGate->getBindUID();
			sprintf_s(msgRet.cIP, sizeof(msgRet.cIP), "%s", pCurGate->getIP());
			CGateServer::SharedGateServer()->sendMsg(&msgRet, sizeof(msgRet), pCurGate->getSessionID() );
			LOGFMTD("tell data svr reconnected ok");
		}

		// send msg to client ;
		stMsgReconnectRet msgback ;
		msgback.nRet = (bReconnectOk ? 0 : 1 ) ;
		CGateServer::SharedGateServer()->sendMsgToClient( (char*)&msgback,sizeof(msgback),pData->_connectID ) ;
		return true ;
	}

	// transfer to center server 
	stGateClient* pDstClient = GetGateClientByNetWorkID(pData->_connectID) ;
	if ( pDstClient == NULL )
	{
		LOGFMTE("can not send message to Center Server , client is NULL or not verified, so close the unknown connect") ;
		CGateServer::SharedGateServer()->GetNetWorkForClients()->ClosePeerConnection(pData->_connectID) ;
		return true ;
	}

	if ( pMsg->nTargetID == 0 )
	{
		LOGFMTE("client send msg = %u , port = %d targetid  is null, uid = %u",pMsg->usMsgType,pMsg->cSysIdentifer,pDstClient->getBindUID());
	}

	CGateServer::SharedGateServer()->sendMsg( pMsg,pData->_len ,pDstClient->getSessionID() );
	return true ;
}

bool CGateClientMgr::OnHeatBeat( CONNECT_ID nNewPeer )
{
	stGateClient* pCurGate = GetGateClientByNetWorkID(nNewPeer);
	if (pCurGate == nullptr)
	{
		LOGFMTE("received heat beat , but gate is nullptr");
		return true;
	}
	pCurGate->doRecivedHeatBet();
	return true;
}

void CGateClientMgr::closeAllClient()
{
	LOGFMTI("close all client peers");
	// remove all connecting ;
	auto iter = m_vSessionGateClient.begin() ;
	for ( ; iter != m_vSessionGateClient.end() ;  )
	{
		// tell other server the peer disconnect
		if (iter->second != nullptr && iter->second->getBindUID() )
		{
			stMsgClientConnectStateChanged msgRet;
			msgRet.nCurState = 2;
			msgRet.nTargetID = iter->second->getBindUID();
			CGateServer::SharedGateServer()->sendMsg( &msgRet, sizeof(msgRet), iter->second->getSessionID() );
			auto nConnectID = iter->second->getNetworkID();
			CGateServer::SharedGateServer()->GetNetWorkForClients()->ClosePeerConnection(nConnectID);
		}
		
		removeActiveClientGate(iter->second) ;
		addToResever(iter->second);

		iter = m_vSessionGateClient.begin() ;
	}
}

void CGateClientMgr::onGateCloseCallBack( stGateClient* pGateClient, bool isWaitReconnect )
{
	if ( isWaitReconnect )
	{
		if ( pGateClient->getBindUID() > 0 )
		{
			LOGFMTD("uid = %u start to wait reconnected",pGateClient->getBindUID() );
			pGateClient->startWaitReconnect();
			return;
		}
	}
	CGateServer::SharedGateServer()->GetNetWorkForClients()->ClosePeerConnection(pGateClient->getNetworkID());
	removeActiveClientGate(pGateClient);
	addToResever(pGateClient);
}

void CGateClientMgr::OnNewPeerConnected(CONNECT_ID nNewPeer, ConnectInfo* IpInfo)
{
	std::string strIP = "ip null";
	if ( IpInfo )
	{
		LOGFMTD("a peer connected ip = %s ,port = %d",IpInfo->strAddress,IpInfo->nPort ) ;
	}
	else
	{
		LOGFMTD("a peer connected ip = NULL" ) ;
	}
	
	auto pGate = getReserverGateClient();
	if (pGate == nullptr)
	{
		pGate = new stGateClient();
	}

	pGate->init(CGateServer::SharedGateServer()->generateSessionID(), nNewPeer,strIP.c_str());
	pGate->setCloseCallBack(std::bind(&CGateClientMgr::onGateCloseCallBack,this,std::placeholders::_1,std::placeholders::_2));
	addClientGate(pGate);
	
	// check pai dui 
	if ( m_vSessionGateClient.size() > MAX_INCOME_PLAYER )
	{
		pGate->delayClose();
		// send msg to tell client svr is full ;
		stMsgGateSvrFull stFull;
		sendMsgToClient(&stFull,sizeof(stFull),pGate->getNetworkID());
		LOGFMTE("gate is full , please try other gate , cnt = %u",m_vSessionGateClient.size());
	}
}

void CGateClientMgr::OnPeerDisconnected( CONNECT_ID nPeerDisconnected, ConnectInfo* IpInfo  )
{
	// client disconnected ;
	stGateClient* pDstClient = GetGateClientByNetWorkID(nPeerDisconnected) ;
	if ( pDstClient == nullptr || pDstClient->isWaitingReconnect() )
	{
		LOGFMTE("gate peer is nullptr , why you get disconnect again ? ");
		return;
	}

	LOGFMTD("one gate peer disconnected");
	onGateCloseCallBack(pDstClient,pDstClient->getBindUID() > 0 );
}

void CGateClientMgr::addClientGate(stGateClient* pGateClient )
{
	auto iter = m_vNetWorkIDGateClientIdx.find(pGateClient->getNetworkID() ) ;
	if ( iter != m_vNetWorkIDGateClientIdx.end() )
	{
		LOGFMTE("why this pos already have data client") ;
		removeActiveClientGate(iter->second);
	}

	auto iterS = m_vSessionGateClient.find(pGateClient->getSessionID()) ;
	if ( iterS != m_vSessionGateClient.end() )
	{
		LOGFMTE("why this pos session id = %d had client data",pGateClient->getSessionID()) ;
		removeActiveClientGate(iter->second);
	}

	m_vNetWorkIDGateClientIdx[pGateClient->getNetworkID()] = pGateClient ;
	m_vSessionGateClient[pGateClient->getSessionID()] = pGateClient ;
}

void CGateClientMgr::removeActiveClientGate(stGateClient* pGateClient )
{
	if ( pGateClient == NULL )
	{
		LOGFMTE("why remove a null client ") ;
		return ;
	}

	MAP_NETWORKID_GATE_CLIENT::iterator iterN = m_vNetWorkIDGateClientIdx.find(pGateClient->getNetworkID()) ;
	if ( iterN != m_vNetWorkIDGateClientIdx.end() )
	{
		m_vNetWorkIDGateClientIdx.erase(iterN) ;
	}
	else
	{
		LOGFMTE("can not find net work id = %d to remove ip = %s",pGateClient->getNetworkID(),pGateClient->getIP() ) ;
	}
	
	MAP_SESSIONID_GATE_CLIENT::iterator iterS = m_vSessionGateClient.find( pGateClient->getSessionID() );
	if ( iterS != m_vSessionGateClient.end() )
	{
		m_vSessionGateClient.erase(iterS) ;
	}
	else
	{
		LOGFMTD("can not find session id = %d to remove ip = %s",pGateClient->getSessionID(),pGateClient->getIP() ) ;
	}
}

stGateClient* CGateClientMgr::getReserverGateClient()
{
	stGateClient* pGateClient = NULL ;
	if ( m_vGateClientReserver.empty() == false )
	{
		auto iter = m_vGateClientReserver.begin();
		pGateClient =  *iter;
		m_vGateClientReserver.erase(iter) ;
	}
	return pGateClient ;
}

stGateClient* CGateClientMgr::getGateClientBySessionID( uint32_t nSessionID)
{
	MAP_SESSIONID_GATE_CLIENT::iterator iter = m_vSessionGateClient.find(nSessionID) ;
	if ( iter == m_vSessionGateClient.end() )
		return NULL ;
	return iter->second ;
}

stGateClient* CGateClientMgr::GetGateClientByNetWorkID(CONNECT_ID& nNetWorkID )
{
	MAP_NETWORKID_GATE_CLIENT::iterator iter = m_vNetWorkIDGateClientIdx.find(nNetWorkID) ;
	if ( iter != m_vNetWorkIDGateClientIdx.end() )
		return iter->second ;
	return NULL ;
}

void CGateClientMgr::addToResever( stGateClient* pClient )
{
	if (pClient == nullptr)
	{
		LOGFMTE("why add a null gate client to reserver");
		return;
	}
	pClient->reset();
	m_vGateClientReserver.insert(m_vGateClientReserver.end(),pClient);
	if (m_vGateClientReserver.size() > 600)
	{
		auto iter = m_vGateClientReserver.begin();
		delete *iter;
		m_vGateClientReserver.erase(iter);
		LOGFMTE("reserve more than 600 , must delte one ");
	}
}

void CGateClientMgr::sendMsgToClient( stMsg* pmsg, uint16_t nLen, CONNECT_ID nNetWorkID )
{
	CGateServer::SharedGateServer()->sendMsgToClient((char*)pmsg, nLen, nNetWorkID);
}
