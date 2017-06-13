#include <WinSock2.h>
#include "ClientNetworkImp.h"
#include "Session.h"
#define CLIENT_HEAT_BET_CHECK_TIEM  (TIME_HEAT_BET + 1 )
CClientNetworkImp::CClientNetworkImp()
	:m_ptrIoWork(nullptr)
{
	m_ptrStrand = nullptr;
	m_ptrSession = nullptr;
	m_nState = eState_None;
}

CClientNetworkImp::~CClientNetworkImp()
{

}

bool CClientNetworkImp::init()
{
	m_ptrStrand = std::make_shared<asio::io_service::strand>(m_ioservice);
	m_ptrSession = nullptr;
	return true ;
}

void CClientNetworkImp::shutdown()
{
	if ( m_nState == eState_Connecting)
	{
		m_ptrSession->closeSession();
		m_ptrIoWork.reset();
		m_ioservice.stop();
		m_nState = eState_None;
		printf("shutdow when connecting\n");
		return;
	}
	m_ioservice.post([this]() { m_ptrSession->closeSession(); m_nState = eState_None; }); 
	m_ptrIoWork.reset();
}

bool CClientNetworkImp::connectToServer(const char* pIP, unsigned short nPort )
{
	if ( m_nState == eState_Connecting || m_nState == eState_Connected )
	{
		printf("connecting or connected , don't do twice\n") ;
		return false;
	}

	if ( nullptr == m_ptrSession )
	{
		if (nullptr == m_ptrStrand)
		{
			m_ptrStrand = std::make_shared<asio::io_service::strand>(m_ioservice);
		}
		
		m_ptrSession = std::make_shared<CSession>(m_ptrStrand);
		m_ptrSession->setRecieveCallBack(std::bind(&CClientNetworkImp::onReivedData, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		m_ptrSession->setErrorCallBack([this]( uint32_t nSessionID ) 
		{
			if ( m_nState != eState_Connected)
			{
				printf("not connected , how to disconnectd \n");
				return;
			}

			m_ptrSession->closeSession();

			Packet* pack = new Packet;
			pack->_brocast = false;
			pack->_packetType = _PACKET_TYPE_DISCONNECT;
			pack->_connectID = nSessionID;
			pack->_len = 0;
			memset(pack->_orgdata, 0, sizeof(pack->_orgdata));
			addPacket(pack);
		}
		);
	}

	m_nState = eState_Connecting;
	tcp::resolver resolver(m_ioservice);
	tcp::resolver::query query(pIP, std::to_string(nPort));
	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	asio::async_connect(m_ptrSession->socket(), endpoint_iterator, [this](const asio::error_code& ec,tcp::resolver::iterator i) {
		bool bSucce = !ec;
		Packet* pack = new Packet;
		pack->_brocast = false;
		pack->_packetType = bSucce ? _PACKET_TYPE_CONNECTED : _PACKET_TYPE_CONNECT_FAILED;
		pack->_connectID = 0;
		if (bSucce)
		{
			m_ptrSession->start();
		}
		else
		{
			printf("connected Failed\n");
		}
		m_nState = bSucce ? eState_Connected : eState_ConnectedFailed;
		addPacket(pack);
	});

	// start the thread 
	if (nullptr == m_ptrIoWork)
	{
		m_ptrIoWork = std::make_shared<asio::io_service::work>(m_ioservice);
		std::thread tThread([this]() { m_ioservice.run(); printf("thread do exit\n"); });
		tThread.detach();
	}
	return true ;
}


bool CClientNetworkImp::getAllPacket(LIST_PACKET& vOutPackets ) // must delete out side ;
{
	std::lock_guard<std::mutex> wLock(m_PacketMutex);
	vOutPackets.swap(m_vRecivedPackets) ;
	return !vOutPackets.empty();
}

bool CClientNetworkImp::getFirstPacket(Packet** ppPacket ) // must delete out side ;
{
	std::lock_guard<std::mutex> wLock(m_PacketMutex);
	if ( m_vRecivedPackets.empty() )
	{
		return false ;
	}

	LIST_PACKET::iterator iter = m_vRecivedPackets.begin() ;
	Packet* p = m_vRecivedPackets.front() ;
	*ppPacket = p ;
	m_vRecivedPackets.erase(iter) ;
	return true ;
}

void CClientNetworkImp::addPacket(Packet* pPacket ) 
{
	std::lock_guard<std::mutex> wLock(m_PacketMutex);
	m_vRecivedPackets.push_back(pPacket) ;
}

bool CClientNetworkImp::sendMsg(const char* pData , size_t nLen ) 
{
	if ( nLen >= CInternalBuffer::max_body_length )
	{
		printf("overflow max msg size = %u  , curSize = %u",CInternalBuffer::max_body_length,nLen) ;
		return false ;
	}
	
	if ( eState_Connected != m_nState)
	{
		printf("not connect , so can not send msg\n");
		return false;
	}
	m_ptrSession->sendData(pData, nLen);
	return true ;
}

void CClientNetworkImp::onReivedData(uint32_t nConnectID, const char* pBuffer, size_t nLen)
{
	if (nLen > _MSG_BUF_LEN)
	{
		printf("too big buffer from connect id = %u\n", nConnectID);
		return;
	}
	Packet* pack = new Packet;
	pack->_brocast = false;
	pack->_packetType = _PACKET_TYPE_MSG;
	pack->_connectID = nConnectID;
	pack->_len = nLen;
	if (pack->_len > sizeof(pack->_orgdata))
	{
		delete pack;
		pack = nullptr;
		printf("too big recve size = %u\n", nLen);
		return;
	}
	memcpy_s(pack->_orgdata, sizeof(pack->_orgdata), pBuffer, pack->_len);
	addPacket(pack);
}