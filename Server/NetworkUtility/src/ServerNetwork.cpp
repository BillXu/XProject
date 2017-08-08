#include "ServerNetwork.h"
#include "SeverNetworkImp.h"
#include "H5ServerNetworkImp.h"
CServerNetwork::CServerNetwork()
{
	m_pNetPeer = NULL;
	m_vAllDelegates.clear() ;

	//zsummer::log4z::ILog4zManager::GetInstance()->Config("server.cfg");
	//zsummer::log4z::ILog4zManager::GetInstance()->Start();
}

CServerNetwork::~CServerNetwork()
{
	ShutDown();
}

bool CServerNetwork::StartupNetwork( unsigned short nPort , int nMaxInComming , bool isNative )
{
	assert(m_pNetPeer == NULL && nMaxInComming > 0 && "m_pNetPeer Must NULL" ) ;
	if ( m_pNetPeer )
	{
		ShutDown();
	}

	if (isNative)
	{
		m_pNetPeer = new CServerNetworkImp();
	}
	else
	{
		m_pNetPeer = new H5ServerNetworkImp();
	}
	
	if (  !m_pNetPeer->init(nPort))
	{
		printf( "Can not Start ServerNetwork \n" );
		return false ;
	}
	return  true;
}

void CServerNetwork::ShutDown()
{
	if ( m_pNetPeer == NULL)
		return ;
	m_vAllDelegates.clear() ;
	delete m_pNetPeer ;
	m_pNetPeer = NULL ;
}

void CServerNetwork::RecieveMsg()
{
	if ( m_pNetPeer == NULL )
	{
		return ;
	}

	CServerNetworkImp::LIST_PACKET vPacket ;
	if ( !m_pNetPeer->getAllPacket(vPacket) )
	{
		return ;
	}

	CServerNetworkImp::LIST_PACKET::iterator iter = vPacket.begin();
	for ( ; iter != vPacket.end(); ++iter )
	{
		Packet* packet = *iter ;
		if ( packet->_packetType == _PACKET_TYPE_CONNECTED )
		{
			EnumDelegate(&CServerNetwork::OnNewPeerConnected,packet);
		}
		else if ( _PACKET_TYPE_DISCONNECT == packet->_packetType )
		{
			EnumDelegate(&CServerNetwork::OnPeerDisconnected,packet);
		}
		else if ( _PACKET_TYPE_MSG == packet->_packetType )
		{
			EnumDelegate(&CServerNetwork::OnLogicMessage,packet);
		}
		delete packet;
	}
}

void CServerNetwork::SendMsg(const char* pData , int nLength , CONNECT_ID& nSendToOrExcpet ,bool bBroadcast )
{
	if ( !m_pNetPeer )
		return ;
	m_pNetPeer->sendMsg(nSendToOrExcpet, pData,nLength);
}

void CServerNetwork::ClosePeerConnection(CONNECT_ID nPeerToClose)
{
	if ( !m_pNetPeer )
		return ;
	m_pNetPeer->closePeerConnection(nPeerToClose);
}

void CServerNetwork::AddDelegate( CServerNetworkDelegate* pDelegate , unsigned int nPriority /* = 0 */ )
{
	m_vAllDelegates.push_back(pDelegate);
}

void CServerNetwork::RemoveDelegate(CServerNetworkDelegate* pDelegate )
{
	LIST_DELEGATE::iterator iter = m_vAllDelegates.begin();
	for ( ; iter != m_vAllDelegates.end(); ++iter )
	{
		if ( (*iter) == pDelegate )
		{
			m_vAllDelegates.erase(iter);
			return ;
		}
	}
}

bool CServerNetwork::OnNewPeerConnected(CServerNetworkDelegate* pDelegate, Packet* pData )
{
	ConnectInfo* info = (ConnectInfo*)pData->_orgdata ;
	pDelegate->OnNewPeerConnected(pData->_connectID,info);
	return true ;
}

bool CServerNetwork::OnPeerDisconnected(CServerNetworkDelegate* pDelegate, Packet* pData )
{
	ConnectInfo* info = (ConnectInfo*)pData->_orgdata ;
	if ( strlen(pData->_orgdata) == 0 )
	{
		info = NULL;
	}
	pDelegate->OnPeerDisconnected(pData->_connectID,info);
	return true ;
}

bool CServerNetwork::OnLogicMessage(CServerNetworkDelegate* pDeleate, Packet* pPacket)
{
	unsigned char cSys = *((unsigned char*)pPacket->_orgdata);
	if ((unsigned char)-1 != cSys) // normal logic msg // heat beat flag ;
	{
		return pDeleate->OnMessage(pPacket);
	}

	// recived heat beat ;
	if ( pDeleate->OnHeatBeat(pPacket->_connectID) )
	{
		//printf("recieved heat beat , send back heat beat \n");
		SendMsg(pPacket->_orgdata, pPacket->_len, pPacket->_connectID);
		return true;
	}
	return false;
}

void CServerNetwork::EnumDelegate( lpFunc pFunc, Packet* pData )
{
	LIST_DELEGATE::iterator iter = m_vAllDelegates.begin();
	for ( ; iter != m_vAllDelegates.end(); ++iter )
	{
		if ((this->*pFunc)(*iter,pData))
		{
			return ;
		}
	}
}
