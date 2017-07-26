//
//  NetWorkManager.cpp
//  God
//
//  Created by Xu BILL on 12-10-30.
//
//
//#ifdef _MSC_VER
//#define _WIN32_WINNT 0x0501
//#endif
 #include <WinSock2.h>
#include "NetWorkManager.h"
#include "ClientNetworkImp.h"
//#include "log4z.h"
//#include "log4z.h"
//#include "MessageDefine.h"
#include "NetworkDefine.h"
int CNetWorkMgr::s_nCurrentDataSize = 0 ;
void CNetMessageDelegate::SetPriority( unsigned int nPriority )
{
	if ( nPriority == GetPriority() )
		return ;
	m_nPriority = nPriority ; 
}

CNetWorkMgr::CNetWorkMgr()
{
    m_pNetPeer = NULL ;
	m_nMaxConnectTo = 0;
	m_nConnectedTo = 0 ;
	m_fHeatBeatElaps = 5;
	m_nCurrentServer = INVALID_CONNECT_ID;
	m_fSendHeatBetTicket = 0;
	m_fCheckHeatBetTicket = 0;
	m_fHeatBeatTimeOut = ( m_fHeatBeatElaps * 2 );
	//zsummer::log4z::ILog4zManager::GetInstance()->Config("client.cfg");
	//zsummer::log4z::ILog4zManager::GetInstance()->Start();
}

CNetWorkMgr::~CNetWorkMgr()
{
	if ( m_pNetPeer )
	{
		m_pNetPeer->shutdown() ;
		delete m_pNetPeer;
		m_pNetPeer = NULL ;
	}
	RemoveAllDelegate() ;
}

void CNetWorkMgr::ShutDown()
{
    if ( m_pNetPeer )
    {
        m_pNetPeer->shutdown();
    }
	m_fCheckHeatBetTicket = 0;
}

//CNetWorkMgr* CNetWorkMgr::SharedNetWorkMgr()
//{
//    static CNetWorkMgr s_gNetWork ;
//    return &s_gNetWork ;
//}

void CNetWorkMgr::SetupNetwork( int nIntendServerCount, float fHeatBeatElapsBySec )
{
	if ( !m_pNetPeer )
	{
		m_nMaxConnectTo = nIntendServerCount ;
		m_pNetPeer =  new CClientNetworkImp() ;
		m_pNetPeer->init();
	}

	m_fHeatBeatElaps = fHeatBeatElapsBySec;
	m_fHeatBeatTimeOut = max( 6, (uint32_t)(m_fHeatBeatElaps * 2));
}

bool CNetWorkMgr::ConnectToServer(const char *pSeverIP, unsigned short nPort , const char* pPassword)
{
	assert(m_pNetPeer && "Pls SetupNetwork() first! " );
    if ( !m_pNetPeer )
	{
		m_pNetPeer =  new CClientNetworkImp() ;
		m_pNetPeer->init();
	}
	assert(m_nConnectedTo < m_nMaxConnectTo && "no more slot for new coming server" );
	if ( m_nMaxConnectTo <= m_nConnectedTo )
	{
		//LOGFMTE("no more slot for new coming server, so can not connected to the server: %s , port: %d",pSeverIP, nPort );
		return false ;
	}

	unsigned short nPasswordLen = 0;
	if ( pPassword && strcmp(pPassword,"0"))
	{
		nPasswordLen = strlen(pPassword) ;
	}
	else
	{
		pPassword = NULL ;
	}

	return m_pNetPeer->connectToServer(pSeverIP, nPort) ;
}

void CNetWorkMgr::ReciveMessage()
{
    ProcessDelegateAddRemove();
	if ( m_pNetPeer == NULL )
		return ;
	
	CClientNetworkImp::LIST_PACKET vPacket ;
	if ( !m_pNetPeer->getAllPacket(vPacket) )
	{
		return ;
	}

	CClientNetworkImp::LIST_PACKET::iterator iter = vPacket.begin();
	for ( ; iter != vPacket.end(); ++iter )
	{
		Packet* packet = *iter ;
		if ( packet->_packetType == _PACKET_TYPE_CONNECTED )
		{
			m_nCurrentServer = packet->_connectID;
			EnumDeleagte(this, (lpfunc)(&CNetWorkMgr::OnConnectSateChanged),packet) ;
		}
		else if ( _PACKET_TYPE_DISCONNECT == packet->_packetType )
		{
			if ( packet->_connectID == m_nCurrentServer )
			{
				m_nCurrentServer = INVALID_CONNECT_ID ;
			}

			EnumDeleagte(this, (lpfunc)(&CNetWorkMgr::OnLostServer),packet) ;
		}
		else if ( _PACKET_TYPE_MSG == packet->_packetType )
		{
			ProcessDelegateAddRemove();
			s_nCurrentDataSize = packet->_len ;
			EnumDeleagte(this, (lpfunc)(&CNetWorkMgr::OnReciveLogicMessage),packet) ;
		}
		else if ( _PACKET_TYPE_CONNECT_FAILED == packet->_packetType )
		{
			EnumDeleagte(this, (lpfunc)(&CNetWorkMgr::OnConnectSateChanged),packet) ;
		}
		delete packet;
	}
}

void CNetWorkMgr::ReciveOneMessage()
{
	ProcessDelegateAddRemove();
	if ( m_pNetPeer == NULL )
		return ;
	Packet* packet = nullptr ;
	
	if ( m_pNetPeer->getFirstPacket(&packet) == false )
	{
		return ;
	}

	if (packet == nullptr )
	{
		return ;
	}

	if ( packet->_packetType == _PACKET_TYPE_CONNECTED )
	{
		m_nCurrentServer = packet->_connectID;
		EnumDeleagte(this, (lpfunc)(&CNetWorkMgr::OnConnectSateChanged),packet) ;
	}
	else if ( _PACKET_TYPE_DISCONNECT == packet->_packetType )
	{
		if ( packet->_connectID == m_nCurrentServer )
		{
			m_nCurrentServer = INVALID_CONNECT_ID ;
		}

		EnumDeleagte(this, (lpfunc)(&CNetWorkMgr::OnLostServer),packet) ;
	}
	else if ( _PACKET_TYPE_MSG == packet->_packetType )
	{
		ProcessDelegateAddRemove();
		s_nCurrentDataSize = packet->_len ;
		EnumDeleagte(this, (lpfunc)(&CNetWorkMgr::OnReciveLogicMessage),packet) ;
	}
	else if ( _PACKET_TYPE_CONNECT_FAILED == packet->_packetType )
	{
		EnumDeleagte(this, (lpfunc)(&CNetWorkMgr::OnConnectSateChanged),packet) ;
	}
	delete packet;
}

void CNetWorkMgr::update(float fDelta)
{
	ReciveMessage();
	updateHeatBeatTimer(fDelta);
}

bool CNetWorkMgr::SendMsg(const char *pbuffer, int iSize)
{
    if ( m_pNetPeer == NULL )
        return false ;
	return m_pNetPeer->sendMsg(pbuffer, iSize) ;
}

bool CNetWorkMgr::SendMsg( const char* pbuffer , int iSize,CONNECT_ID& nServerNetUID )
{
	if ( m_pNetPeer == NULL || nServerNetUID == INVALID_CONNECT_ID )
		return false ;
	return m_pNetPeer->sendMsg(pbuffer, iSize) ;
}

void CNetWorkMgr::AddMessageDelegate(CNetMessageDelegate *pDelegate, unsigned short nPrio )
{
	if ( !pDelegate)
		return ;
	pDelegate->SetNetWorkMgr(this) ;
	pDelegate->SetPriority(nPrio) ;
    m_vWillAddDelegate.push_back(pDelegate) ;
//    printf("\n加入了一个delegate.");
}

void CNetWorkMgr::AddMessageDelegate(CNetMessageDelegate *pDelegate )
{
    m_vWillAddDelegate.push_back(pDelegate) ;
}

void CNetWorkMgr::RemoveAllDelegate()
{
    m_vAllDelegate.clear() ;
}

void CNetWorkMgr::RemoveMessageDelegate(CNetMessageDelegate *pDelegate)
{
    if ( pDelegate )
    {
        m_vWillRemoveDelegate.push_back(pDelegate) ;
//        printf("\n移除了一个delegate.");
    }
}

bool CNetWorkMgr::OnLostServer( CNetMessageDelegate* pDeleate,void* pData )
{
    return pDeleate->OnLostSever((Packet*)pData) ;
}

bool CNetWorkMgr::OnReciveLogicMessage( CNetMessageDelegate* pDeleate,void* pData )
{
	auto pPacket = (Packet*)pData;
	unsigned char cSys = *((unsigned char*)pPacket->_orgdata);
	if ((unsigned char)-1 != cSys ) // normal logic msg // heat beat flag ;
	{
		return pDeleate->OnMessage(pPacket);
	}

	// recived heat beat ;
	m_fCheckHeatBetTicket = 0;  // reset check heat bet ;
	unsigned char* p = (unsigned char*)pPacket->_orgdata;
	++p;
	uint32_t nHeatBetNum = *((uint32_t*)p);
	return pDeleate->onHeatBeat(nHeatBetNum);
}

void CNetWorkMgr::EnumDeleagte( CNetWorkMgr* pTarget, lpfunc pFunc, void* pData )
{
    LIST_DELEGATE::iterator iter = m_vAllDelegate.begin() ;
    for ( ; iter != m_vAllDelegate.end(); ++iter )
    {
        if ( (pTarget->*pFunc)(*iter,pData))
            return ;
    }
}

void CNetWorkMgr::doSendHeatBeat()
{
	if (m_nCurrentServer == INVALID_CONNECT_ID)
	{
		return;
	}

	unsigned char p[sizeof(unsigned char) + sizeof(uint32_t)] = {0};
	(*p) = -1;  // heat beat flag ;
	*((uint32_t*)(p + 1 )) = (uint32_t)time(nullptr);
	SendMsg((char*)p, sizeof(p));
}

void CNetWorkMgr::DisconnectServer()
{
	if (m_pNetPeer)
	{
		m_pNetPeer->closeSession();
	}
}

bool CNetWorkMgr::OnConnectSateChanged( CNetMessageDelegate* pDeleate,void* pData )
{
	Packet* packet = (Packet*)pData ;
    //unsigned char nMessageID = packet->data[0];
    CNetMessageDelegate::eConnectState eSate = CNetMessageDelegate::eConnect_Accepted;
     switch (packet->_packetType)
     {
         case _PACKET_TYPE_CONNECTED:
         {
             eSate = CNetMessageDelegate::eConnect_Accepted;
         }
             break ;
         case _PACKET_TYPE_CONNECT_FAILED:
         {
            eSate = CNetMessageDelegate::eConnect_Failed;
         }
             break ;
//         case ID_NO_FREE_INCOMING_CONNECTIONS:
//         {
//             eSate = CNetMessageDelegate::eConnect_SeverFull;
//         }
//             break ;
//         case ID_CONNECTION_BANNED:
//         {
//             eSate = CNetMessageDelegate::eConnect_Banned;
//         }
//             break ;
// 		case ID_ALREADY_CONNECTED:
// 		{
// 			eSate = CNetMessageDelegate::eConnect_AlreadyConnected;
// 		}
// 			break;
         default:
             return true ;
     }
	 return pDeleate->OnConnectStateChanged(eSate,packet) ;
}

void CNetWorkMgr::ProcessDelegateAddRemove()
{
    if ( m_vWillRemoveDelegate.size() > 0 )
    {
        LIST_DELEGATE::iterator iter = m_vWillRemoveDelegate.begin();
        for ( ; iter != m_vWillRemoveDelegate.end() ; ++iter )
        {
            LIST_DELEGATE::iterator iterRemove = m_vAllDelegate.begin() ;
            for ( ; iterRemove != m_vAllDelegate.end() ; ++iterRemove )
            {
                if ( *iterRemove == *iter )
                {
                    m_vAllDelegate.erase(iterRemove) ;
                    break ;
                }
            }
        }
        m_vWillRemoveDelegate.clear();
    }
    
    if ( m_vWillAddDelegate.size() > 0 )
    {
        LIST_DELEGATE::iterator iter = m_vWillAddDelegate.begin();
        CNetMessageDelegate* pDelegate = NULL ;
        for ( ; iter != m_vWillAddDelegate.end() ; ++iter )
        {
            pDelegate = *iter ;
            if ( !pDelegate)
                continue;
			if ( IsAlreadyAdded(pDelegate) )
			{
				//LOGFMTD("duplicate add network delegate") ;
				continue;
			}
            pDelegate->SetNetWorkMgr(this) ;
            LIST_DELEGATE::iterator iter = m_vAllDelegate.begin();
            CNetMessageDelegate* pDelegateIter = NULL ;
            bool bInsert = false ;
            for ( ; iter != m_vAllDelegate.end(); ++iter)
            {
                pDelegateIter = *iter ;
                if ( pDelegateIter->GetPriority() <= pDelegate->GetPriority() )
                {
                    m_vAllDelegate.insert(iter,pDelegate);
                    bInsert = true ;
                    break ;
                }
            }
            
            if ( bInsert == false )
            {
                m_vAllDelegate.push_back(pDelegate) ;
            }
        }
        m_vWillAddDelegate.clear();
    }
}

bool CNetWorkMgr::IsAlreadyAdded(CNetMessageDelegate*pDeleate)
{
	LIST_DELEGATE::iterator iter = m_vAllDelegate.begin();
	for ( ;iter != m_vAllDelegate.end(); ++iter )
	{
		if ( *iter == pDeleate )
		{
			return true ;
		}
	}
	return false ;
}

void CNetWorkMgr::updateHeatBeatTimer(float fDeta)
{
	if ( INVALID_CONNECT_ID == m_nCurrentServer)
	{
		return;
	}

	m_fCheckHeatBetTicket += fDeta;
	m_fSendHeatBetTicket += fDeta;

	if ( m_fCheckHeatBetTicket > m_fHeatBeatTimeOut )  // check is wait too long not recived svr heat bet ;
	{
		printf("heat beat time out svr disconnect\n");
		m_nCurrentServer = INVALID_CONNECT_ID;
		m_fCheckHeatBetTicket = 0;
		m_fSendHeatBetTicket = 0;
		EnumDeleagte(this, (lpfunc)(&CNetWorkMgr::OnLostServer), nullptr );
		if ( m_pNetPeer )
		{
			m_pNetPeer->closeSession();
		}
		return;
	}

	if ( m_fSendHeatBetTicket >= m_fHeatBeatElaps )
	{
		doSendHeatBeat();
		m_fSendHeatBetTicket -= m_fHeatBeatElaps;
	}
}


