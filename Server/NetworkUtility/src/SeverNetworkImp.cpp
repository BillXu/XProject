 #include <WinSock2.h>
#include "SeverNetworkImp.h"
#include "Session.h"
CServerNetworkImp::CServerNetworkImp()
{
	m_acceptor = nullptr ;
}

CServerNetworkImp::~CServerNetworkImp()
{
	m_ioService.stop() ;
	if ( m_acceptor )
	{
		delete m_acceptor ;
	}

}

bool CServerNetworkImp::init(uint16_t nPort )
{
	m_ptrStrand = std::make_shared<asio::io_service::strand>(m_ioService);

	tcp::endpoint endpoint(tcp::v4(), nPort);
	m_acceptor = new tcp::acceptor(m_ioService,endpoint);
	startAccept();

	m_pIOThread = std::make_shared<std::thread>([this] { m_ioService.run(); });
	m_pIOThread->detach();

	m_pIOThread = std::make_shared<std::thread>([this] { m_ioService.run(); });
	m_pIOThread->detach();
	LOGFMTI("init server net imp, port = %u",nPort);
	return true ;
}

void CServerNetworkImp::startAccept() 
{
	LOGFMTD("startAccept");
	auto ptrSession = std::make_shared<CSession>(m_ptrStrand);
	m_acceptor->async_accept(ptrSession->socket(), m_ptrStrand->wrap([this, ptrSession](const asio::error_code& error) { handleAccept(error,ptrSession); }));
}

void CServerNetworkImp::handleAccept( const asio::error_code& error, session_ptr session )
{
	if (!error)  
	{  
		session->setErrorCallBack(std::bind(&CServerNetworkImp::doCloseSession,this,std::placeholders::_1,false));
		session->setRecieveCallBack(std::bind(&CServerNetworkImp::onReivedData,this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		session->start();
		
		m_SessionMutex.lock();
		m_vActiveSessions[session->getConnectID()] = session;
		m_SessionMutex.unlock();

		std::string str = session->socket().remote_endpoint().address().to_string();
		LOGFMTD("a peer connected ip = %s id = %u", str.c_str(), session->getConnectID());
		Packet* pack = new Packet ;
		pack->_brocast = false ;
		pack->_packetType = _PACKET_TYPE_CONNECTED ;
		pack->_connectID = session->getConnectID() ;
		pack->_len = str.size();
		memset(pack->_orgdata,0,sizeof(pack->_orgdata));
		memcpy_s(pack->_orgdata, sizeof(pack->_orgdata), str.c_str(), pack->_len);
		addPacket(pack);
  
	}  
	else
	{
		LOGFMTE("handle accpet error");
	}
	startAccept(); //每连接上一个socket都会调用  
}


void CServerNetworkImp::doCloseSession( uint32_t nConnectID , bool bServerClose )
{
	LOGFMTD("begin close connectID = %u",nConnectID);
	{
		std::lock_guard<std::mutex> tLock(m_SessionMutex);
		auto iter = m_vActiveSessions.find(nConnectID);
		if (iter == m_vActiveSessions.end())
		{
			LOGFMTD("can not find  close connectID = %u", nConnectID);
			return;
		}
		iter->second->closeSession();
		m_vActiveSessions.erase(iter);
	}

	//if ( bServerClose == false ) //  always post this disconnect notice 
	{
		Packet* pack = new Packet();
		pack->_brocast = false;
		pack->_packetType = _PACKET_TYPE_DISCONNECT;
		pack->_connectID = nConnectID;
		pack->_len = 0;
		memset(pack->_orgdata, 0, sizeof(pack->_orgdata));
		addPacket(pack);
	}
	LOGFMTD("after closeSession end id = %u", nConnectID);
}

bool CServerNetworkImp::sendMsg(uint32_t nConnectID , const char* pData , size_t nLen )
{
	session_ptr pt = getSessionByConnectID(nConnectID) ;
	if ( pt )
	{
		return pt->sendData(pData,nLen) ;
	}
	LOGFMTE("cant not find session with id = %d to send msg ", nConnectID);
	return false ;
}

CServerNetworkImp::session_ptr CServerNetworkImp::getSessionByConnectID(uint32_t nConnectID )
{
	std::lock_guard<std::mutex> tLock(m_SessionMutex);
	MAP_SESSION::iterator iter = m_vActiveSessions.find(nConnectID) ;
	if ( iter != m_vActiveSessions.end() )
	{
		return iter->second ;
	}
	LOGFMTE("cant not find session with id = %d",nConnectID ) ;
	return nullptr ;
}

bool CServerNetworkImp::getAllPacket(LIST_PACKET& vOutPackets ) // must delete out side ;
{
	std::lock_guard<std::mutex> tLock(m_PacketMutex);
	vOutPackets.swap(m_vRecivedPackets) ;
	return !vOutPackets.empty();
}

bool CServerNetworkImp::getFirstPacket(Packet** ppPacket ) // must delete out side ;
{
	std::lock_guard<std::mutex> tLock(m_PacketMutex);
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

void CServerNetworkImp::closePeerConnection( uint32_t nConnectID )
{
	LOGFMTD("post close id = %u", nConnectID );
	auto p = std::bind(&CServerNetworkImp::doCloseSession, this, nConnectID, true);
	//	m_ptrStrand->wrap([this, ptrSession](const asio::error_code& error) { handleAccept(error, ptrSession); })
	m_ioService.post(m_ptrStrand->wrap(p));
}

void CServerNetworkImp::addPacket(Packet* pPacket )
{	
	std::lock_guard<std::mutex> tLock(m_PacketMutex);
	m_vRecivedPackets.push_back(pPacket) ;
}

void CServerNetworkImp::onReivedData(uint32_t nConnectID , const char* pBuffer , size_t nLen )
{
	if ( nLen > _MSG_BUF_LEN )
	{
		LOGFMTE("too big buffer from connect id = %u", nConnectID);
		return ;
	}
	Packet* pack = new Packet ;
	pack->_brocast = false ;
	pack->_packetType = _PACKET_TYPE_MSG ;
	pack->_connectID = nConnectID ;
	pack->_len = nLen ;
	if ( pack->_len > sizeof(pack->_orgdata))
	{
		delete pack;
		pack = nullptr;
		LOGFMTE("too big recve size = %u", nLen);
		return;
	}
	memcpy_s(pack->_orgdata, sizeof(pack->_orgdata), pBuffer, pack->_len);
	addPacket(pack);
}