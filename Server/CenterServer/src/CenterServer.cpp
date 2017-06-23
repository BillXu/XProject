#include <windows.h>
#include "CenterServer.h"
#include "log4z.h"
#include "ServerMessageDefine.h"
#include <synchapi.h>
#include <time.h>
#include "ServerConfig.h"
CCenterServerApp* CCenterServerApp::s_GateServer = NULL ;
CCenterServerApp* CCenterServerApp::SharedCenterServer()
{
	return s_GateServer;
}

CCenterServerApp::CCenterServerApp()
{
	m_pNetwork = NULL ;
}

CCenterServerApp::~CCenterServerApp()
{
	if ( m_pNetwork )
	{
		m_pNetwork->ShutDown();
		delete m_pNetwork ;
	}
	m_pNetwork = NULL ;
	s_GateServer = NULL ;
}

bool CCenterServerApp:: Init()
{
	if ( s_GateServer )
	{
		LOGFMTE("can not have too instance of CenterServerApp") ;
		return false ;
	}
	s_GateServer = this ;
	m_bRunning = true ;

	CSeverConfigMgr sg;
	sg.LoadFile("../configFile/serverConfig.txt");

	stServerConfig* pSvrConfig = sg.GetServerConfig(eSvrType_Center);
	if ( pSvrConfig == NULL )
	{
		LOGFMTE("can not find center server config so start up failed ") ;
		return  false ;
	}
	m_pNetwork = new CServerNetwork() ;
	m_pNetwork->StartupNetwork( pSvrConfig->nPort,120,pSvrConfig->strPassword);
	m_pNetwork->AddDelegate(this);

	for ( uint16_t ndx = ID_MSG_PORT_NONE ; ndx < ID_MSG_PORT_MAX ; ++ndx )
	{
		m_vTargetServers[ndx].init((eMsgPort)ndx,2) ;
		LOGFMTE("temp set max svr cnt = 2");
	}

	m_tWaitSvrSetup.setIsAutoRepeat(false);
	m_tWaitSvrSetup.setInterval(60 * 5);
	m_tWaitSvrSetup.setCallBack([this](CTimer* p, float fDelta) 
	{  
		for ( auto& ref : m_vTargetServers)
		{
			if (ref.getPortType() == ID_MSG_PORT_CENTER || ref.getPortType() == ID_MSG_PORT_CLIENT)
			{
				continue;
			}

			if (ref.isHaveEmptySlot())
			{
				LOGFMTE("svr port = %s is not full , not all start up", ref.getPortDesc());
			}
		}
	}
	);
	m_tWaitSvrSetup.start();
	LOGFMTI("start center server !");
	return true ;
}

void  CCenterServerApp::RunLoop()
{
	clock_t t = clock();
	while ( m_bRunning )
	{
		if ( m_pNetwork )
		{
			m_pNetwork->RecieveMsg();
		}

		clock_t tNow = clock();
		float fDelta = float(tNow - t) / CLOCKS_PER_SEC;
		t = tNow;
		CTimerManager::getInstance()->Update(fDelta);
		Sleep(2);
	}

	if ( m_pNetwork )
	{
		m_pNetwork->ShutDown() ;
	}
}

void  CCenterServerApp::Stop()
{
	m_bRunning = false ;
}

bool  CCenterServerApp::OnMessage( Packet* pData )
{
	stMsg* pMsg =(stMsg*)pData->_orgdata ;
	switch ( pMsg->usMsgType )
	{
	case MSG_TRANSER_DATA:
	{
		stMsgTransferData* pTransfer = (stMsgTransferData*)pData->_orgdata;
		stMsg* pReal = (stMsg*)(pData->_orgdata + sizeof(stMsgTransferData));
		auto nPort = pReal->cSysIdentifer;
		if (nPort == ID_MSG_PORT_CLIENT)
		{
			nPort = ID_MSG_PORT_GATE;
		}
		
		if (nPort >= ID_MSG_PORT_MAX)
		{
			LOGFMTE("invalid target port = %s , msgType = %u , target id = %u", getServerDescByType(pReal->cSysIdentifer),pReal->usMsgType,pReal->nTargetID );
			return true;
		}

		std::vector<CONNECT_ID> vTargetPortIDs;
		if ((unsigned int)-1 == pReal->nTargetID)
		{
			m_vTargetServers[nPort].getAllNetworkIDs(vTargetPortIDs);
		}
		else
		{
			auto nNetID = m_vTargetServers[nPort].getNetworkIDByTargetID(pReal->nTargetID);
			vTargetPortIDs.push_back(nNetID);
		}
		
		if ( vTargetPortIDs.empty() )
		{
			LOGFMTE("all svr not connect for port = %s , msg = %u , target id = %u", getServerDescByType(pReal->cSysIdentifer), pReal->usMsgType, pReal->nTargetID );
			return true;
		}

		for (auto& nNetID : vTargetPortIDs)
		{
			if ( INVALID_CONNECT_ID == nNetID )
			{
				LOGFMTE("svr not connect ,invalid nNetID for port = %s , msg = %u , target id = %u", getServerDescByType(pReal->cSysIdentifer), pReal->usMsgType, pReal->nTargetID);
				return true;
			}
			m_pNetwork->SendMsg(pData->_orgdata, pData->_len, nNetID);
		}
	}
	break;
	case MSG_VERIFY_SERVER:
	{
		auto iter = m_vWaitVerifyServerInfo.find(pData->_connectID);
		if (iter == m_vWaitVerifyServerInfo.end() || iter->second == nullptr)
		{
			LOGFMTE("this peer not connected , how ");
			m_pNetwork->ClosePeerConnection(pData->_connectID);
			return true;
		}
		auto pSvr = iter->second;
		m_vWaitVerifyServerInfo.erase(iter);

		stMsgVerifyServer* pVerify = (stMsgVerifyServer*)pMsg;
		if (pVerify->nSeverPortType >= ID_MSG_PORT_MAX)
		{
			LOGFMTE("invalid verify svr port type = %u ip = %s", pVerify->nSeverPortType, pSvr->getIP());
			delete pSvr;
			m_pNetwork->ClosePeerConnection(pData->_connectID);
			return true;
		}

		stMsgVerifyServerRet msgBack;
		msgBack.cSysIdentifer = pVerify->nSeverPortType;
		msgBack.nTargetID = 0;
		msgBack.nRet = 0;
		msgBack.isReconnect = pVerify->isReconnect;
		msgBack.uMaxSvrCount = m_vTargetServers[pVerify->nSeverPortType].getSvrCnt();
		uint16_t nTargetIdx = -1;
		if ( pVerify->isReconnect )
		{
			nTargetIdx = pVerify->nPreIdx;
		}

		if (m_vTargetServers[pVerify->nSeverPortType].addServer(pSvr, nTargetIdx))
		{
			msgBack.uIdx = pSvr->getIdx();
		}
		else
		{
			msgBack.nRet = 1;
		}

		m_pNetwork->SendMsg((char*)&msgBack, sizeof(msgBack), pData->_connectID);
		if (msgBack.nRet)
		{
			LOGFMTE("current svr is full can not more svr ip = %s , port = %u", pSvr->getIP(), pVerify->nSeverPortType);
			delete pSvr;
			m_pNetwork->ClosePeerConnection(pData->_connectID);
			return true;
		}
		pSvr->onVerifiedSvr();
	}
	break;
	default:
		{
			LOGFMTE("unknown msg id = %d , csysIdentifer = %s close the session",pMsg->usMsgType, getServerDescByType(pMsg->cSysIdentifer)) ;
			m_pNetwork->ClosePeerConnection(pData->_connectID);
		}
	}
	return true ;
}

void  CCenterServerApp::OnNewPeerConnected( CONNECT_ID nNewPeer, ConnectInfo* IpInfo )
{
	if ( IpInfo )
	{
		LOGFMTI("a peer connected ip = %s , port = %d connect id = %d",IpInfo->strAddress,IpInfo->nPort ,nNewPeer );
	}
	else
	{
		LOGFMTI("a peer connected ip = NULL, connect id = %d", nNewPeer) ;
	}

	auto p = new SeverInfo();
	p->reset();
	p->setInfo(0, nNewPeer);
	if (IpInfo)
	{
		p->setIp((const char*)IpInfo->strAddress);
	}

	if (m_vWaitVerifyServerInfo.find(nNewPeer) != m_vWaitVerifyServerInfo.end())
	{
		LOGFMTE("why already add this connected ?");
		return;
	}

	m_vWaitVerifyServerInfo[p->getNetworkID()] = p;
	p->startWait();
}

void  CCenterServerApp::OnPeerDisconnected( CONNECT_ID nPeerDisconnected, ConnectInfo* IpInfo )
{
	auto iter = m_vWaitVerifyServerInfo.find(nPeerDisconnected);
	if ( iter != m_vWaitVerifyServerInfo.end())
	{
		LOGFMTE("wait verify svr disconected ip = %s",iter->second->getIP() );
		delete iter->second;
		m_vWaitVerifyServerInfo.erase(iter);
		return;
	}

	// check server dis connect ;
	for (auto& ref : m_vTargetServers)
	{
		auto pSvr = ref.getServerByConnectID(nPeerDisconnected);
		if (pSvr)
		{
			onSvrPortDisconnect(ref.getPortType(),pSvr->getIdx(),ref.getPortType() );
			ref.removeServer(nPeerDisconnected);
			return;
		}
	}

	if ( IpInfo )
	{
		LOGFMTE( "terrible error : not connect but disconnect , a unknown peer dis conntcted ip = %s port = %d",IpInfo->strAddress,IpInfo->nPort ) ;
	}
	else
	{
		LOGFMTE("terrible error : not connect but disconnect  a unknown peer disconnect ip = unknown") ;
	}
}

const char* CCenterServerApp::getServerDescByType( uint16_t eType)
{
	static const char* vSvrString[] =
	{
		"ID_MSG_PORT_NONE" , // client to game server 
		"ID_MSG_PORT_CLIENT",
		"ID_MSG_PORT_GATE",
		"ID_MSG_PORT_CENTER",
		"ID_MSG_PORT_LOGIN",
		"ID_MSG_PORT_VERIFY",
		"ID_MSG_PORT_APNS",
		"ID_MSG_PORT_LOG",
		"ID_MSG_PORT_DATA",
		"ID_MSG_PORT_TAXAS",
		"ID_MSG_PORT_DB",
		"ID_MSG_PORT_ALL_SERVER",
		"ID_MSG_PORT_NIU_NIU",
		"ID_MSG_PORT_GOLDEN",
		"ID_MSG_PORT_MJ",
		"ID_MSG_PORT_MAX",
	};

	if ( eType >= ID_MSG_PORT_MAX)
	{
		return "unknown server type ";
	}
	return vSvrString[eType];
}

void CCenterServerApp::onSvrPortDisconnect( eMsgPort nSvrPort, uint16_t nIdx, uint16_t nMaxSvrCnt )
{
	for (auto& ref : m_vTargetServers)
	{
		if (nSvrPort == ref.getPortType())
		{
			continue;
		}

		std::vector<CONNECT_ID> vConnect;
		ref.getAllNetworkIDs(vConnect);
		if ( vConnect.empty() )
		{
			continue;
		}

		stMsgServerDisconnect msg;
		msg.cSysIdentifer = ref.getPortType();
		msg.nDisconnectPort = nSvrPort;
		msg.nMaxPortCnt = nMaxSvrCnt;
		msg.nPortIdx = nIdx;
		
		for (auto& nNetID : vConnect)
		{
			m_pNetwork->SendMsg( (const char*)&msg, sizeof(msg), nNetID );
		}
	}

	LOGFMTD("send svr dis connect msg port id = %s idx = %d to other svr ", getServerDescByType(nSvrPort),nIdx);
}

void CCenterServerApp::closeConnection(CONNECT_ID nNetID)
{
	m_pNetwork->ClosePeerConnection(nNetID);
}

bool CCenterServerApp::OnHeatBeat( CONNECT_ID nNewPeer )
{
	for (auto& ref : m_vTargetServers)
	{
		auto pSvr = ref.getServerByConnectID(nNewPeer);
		if (pSvr)
		{
			pSvr->onReciveHeatBeat();
			return true;
		}
	}
	return false;
}

void CCenterServerApp::sendMsg(const char* pmsg, uint16_t nLen, CONNECT_ID nTargetID)
{
	m_pNetwork->SendMsg(pmsg, nLen, nTargetID);
}