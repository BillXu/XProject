#include "H5ServerNetworkImp.h"
#include "../../ServerCommon/log4z.h"
H5ServerNetworkImp::H5ServerNetworkImp()
{

}

H5ServerNetworkImp::~H5ServerNetworkImp()
{
	m_pNetServer.get_io_service().stop();
}

bool H5ServerNetworkImp::init(uint16_t nPort)
{
	m_pNetServer.clear_access_channels(websocketpp::log::alevel::all);
	m_pNetServer.clear_error_channels(websocketpp::log::alevel::all);
	//m_pNetServer.clear_access_channels(websocketpp::log::alevel::frame_payload);

	// Initialize Asio
	m_pNetServer.init_asio();

	// Register our message handler
	m_pNetServer.set_open_handler(std::bind(&H5ServerNetworkImp::on_open, this, std::placeholders::_1 ));
	m_pNetServer.set_close_handler(std::bind(&H5ServerNetworkImp::on_close, this, std::placeholders::_1,false ) );
	m_pNetServer.set_message_handler(std::bind(&H5ServerNetworkImp::on_message,this, std::placeholders::_1, std::placeholders::_2));

	// Listen on port 
	m_pNetServer.listen(nPort);

	// Start the server accept loop
	m_pNetServer.start_accept();

	std::thread tThread([this]() { m_pNetServer.run(); printf("thread do exit\n"); });
	tThread.detach();
	
	return true;
}

bool H5ServerNetworkImp::sendMsg(uint32_t nConnectID, const char* pData, size_t nLen)
{
	try
	{
		std::lock_guard<std::mutex> tLock(m_SessionMutex);
		auto iter = m_vActiveSessions.find(nConnectID);
		if (iter == m_vActiveSessions.end())
		{
//#ifdef _DEBUG
			LOGFMTE("send msg to netID = %u , target is null", nConnectID);
//#endif // _DEBUG
			return false;
		}

		m_pNetServer.send(iter->second, pData, nLen, websocketpp::frame::opcode::text);
		return true;
	}
	catch ( websocketpp::exception& ec )
	{
		std::string str(pData, nLen);
		LOGFMTE("send msg error : %s , --what : %s ; conetn : %s", ec.m_msg.c_str(), ec.what(), str.c_str());
	}

	return true;
}

bool H5ServerNetworkImp::getAllPacket(LIST_PACKET& vOutPackets)
{
	std::lock_guard<std::mutex> tLock(m_PacketMutex);
	vOutPackets.swap(m_vRecivedPackets);
	return !vOutPackets.empty();
}

bool H5ServerNetworkImp::getFirstPacket(Packet** ppPacket)
{
	std::lock_guard<std::mutex> tLock(m_PacketMutex);
	if (m_vRecivedPackets.empty())
	{
		return false;
	}

	LIST_PACKET::iterator iter = m_vRecivedPackets.begin();
	Packet* p = m_vRecivedPackets.front();
	*ppPacket = p;
	m_vRecivedPackets.erase(iter);
	return true;
}

void H5ServerNetworkImp::closePeerConnection(uint32_t nConnectID)
{
//#ifdef _DEBUG
	LOGFMTD("post close id = %u", nConnectID);
//#endif // _DEBUG
	
	//auto p = std::bind(&H5ServerNetworkImp::close_handler, this, nConnectID);
	//	m_ptrStrand->wrap([this, ptrSession](const asio::error_code& error) { handleAccept(error, ptrSession); })
	m_pNetServer.get_io_service().post([this,nConnectID]() 
	{
		auto piter = m_vActiveSessions.find(nConnectID);
		if ( piter == m_vActiveSessions.end())
		{
//#ifdef _DEBUG
			LOGFMTE("why nconnect id = %u is null ?", nConnectID);
//#endif // _DEBUG
			return;
		}
		on_close(piter->second,true );
	}
	);
}

void H5ServerNetworkImp::addPacket(Packet* pPacket)
{
	std::lock_guard<std::mutex> tLock(m_PacketMutex);
	m_vRecivedPackets.push_back(pPacket);
}

void H5ServerNetworkImp::on_close( websocketpp::connection_hdl hdl, bool isServerClose )
{
	auto nConnectID = 0;
//#ifdef _DEBUG
	LOGFMTD("begin close connectID = %u", nConnectID);
//#endif // _DEBUG
	{
		std::lock_guard<std::mutex> tLock(m_SessionMutex);
		auto iter = m_vActiveSessions.begin();
		for (; iter != m_vActiveSessions.end(); ++iter)
		{
			if (hdl.lock().get() == iter->second.lock().get() )
			//if ( m_pNetServer.get_con_from_hdl(hdl) == m_pNetServer.get_con_from_hdl(iter->second) )
			{
				nConnectID = iter->first;
				m_vActiveSessions.erase(iter);
				break;
			}
		}
		if ( nConnectID == 0 )
		{
#ifdef _DEBUG
			LOGFMTD("can not find  close connectID = %u", nConnectID);
#endif // _DEBUG
			return;
		}
		
	}

//#ifdef _DEBUG
	LOGFMTD("do close net id = %u", nConnectID);
//#endif // _DEBUG
	
	if ( isServerClose == false ) //  always post this disconnect notice 
	{
		Packet* pack = new Packet();
		pack->_brocast = false;
		pack->_packetType = _PACKET_TYPE_DISCONNECT;
		pack->_connectID = nConnectID;
		pack->_len = 0;
		memset(pack->_orgdata, 0, sizeof(pack->_orgdata));
		addPacket(pack);
//#ifdef _DEBUG
		LOGFMTD("client invoker after closeSession end id = %u", nConnectID);
//#endif // _DEBUG
		
	}
	else
	{
		m_pNetServer.close(hdl, websocketpp::close::status::normal, "invalid");
//#ifdef _DEBUG
		LOGFMTD("svr do client after closeSession end id = %u", nConnectID);
//#endif // _DEBUG
	}
	
}

void H5ServerNetworkImp::on_open(websocketpp::connection_hdl hdl)
{
	if ( nullptr == m_pNetServer.get_con_from_hdl(hdl) )
	{
//#ifdef _DEBUG
		LOGFMTE("new connect connection , get cano get data info");
//#endif // _DEBUG
		return;
	}

	m_SessionMutex.lock();
	auto netID= ++m_nCurMaxNetID;
	if (m_vActiveSessions.find(netID) != m_vActiveSessions.end())
	{
//#ifdef _DEBUG
		LOGFMTE("why have double netid = %u", netID);
//#endif // _DEBUG
		m_SessionMutex.unlock();
		assert(0 && "duplicate net id " );
		return;
	}
	m_vActiveSessions[netID] = hdl;
	m_SessionMutex.unlock();

	auto pRpoint = m_pNetServer.get_con_from_hdl(hdl)->get_socket().remote_endpoint();
	std::string str = pRpoint.address().to_string();
	auto nPos = str.find_last_of(':');
	if (nPos != std::string::npos)
	{
		str = str.substr(nPos + 1, str.size() - nPos);
	}
	
//#ifdef _DEBUG
	LOGFMTD("a peer connected ip = %s , port = %u id = %u", str.c_str(), pRpoint.port(), netID);
//#endif // _DEBUG
	Packet* pack = new Packet;
	pack->_brocast = false;
	pack->_packetType = _PACKET_TYPE_CONNECTED;
	pack->_connectID = netID;
	pack->_len = str.size();
	memset(pack->_orgdata, 0, sizeof(pack->_orgdata));

	ConnectInfo* pCInfo = (ConnectInfo*)pack->_orgdata;
	auto nBufferSize = sizeof(pCInfo->strAddress) - 1;
	memcpy_s(pCInfo->strAddress, nBufferSize, str.c_str(), (nBufferSize > str.size() ? str.size() : nBufferSize ) );
	pCInfo->nPort = pRpoint.port();
	addPacket(pack);
}

void H5ServerNetworkImp::on_message(websocketpp::connection_hdl hdl, message_ptr msg)
{
	auto& pData = msg->get_payload();
	if ( pData.size()> _MSG_BUF_LEN)
	{
//#ifdef _DEBUG
		LOGFMTE("too big buffer from connect id = %u", (uint32_t)hdl.lock().get());
//#endif // _DEBUG
		return;
	}
	Packet* pack = new Packet;
	pack->_brocast = false;
	pack->_packetType = _PACKET_TYPE_MSG;
	pack->_connectID = getNetID(hdl);
	pack->_len = pData.size();
	if (pack->_len > sizeof(pack->_orgdata))
	{
		delete pack;
		pack = nullptr;
//#ifdef _DEBUG
		LOGFMTE("too big recve size = %u", pData.size());
//#endif // _DEBUG
		return;
	}
	memcpy_s(pack->_orgdata, sizeof(pack->_orgdata), pData.c_str(), pack->_len);
	addPacket(pack);
}

uint32_t H5ServerNetworkImp::getNetID( websocketpp::connection_hdl hdl )
{
	std::lock_guard<std::mutex> tLock(m_SessionMutex);
	auto iter = m_vActiveSessions.begin();
	for (; iter != m_vActiveSessions.end(); ++iter)
	{
		if (hdl.lock().get() == iter->second.lock().get())
		{
			return iter->first;
		}
	}
	return 0;
}