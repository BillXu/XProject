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
	m_pNetServer.set_access_channels(websocketpp::log::alevel::all);
	m_pNetServer.clear_access_channels(websocketpp::log::alevel::frame_payload);

	// Initialize Asio
	m_pNetServer.init_asio();

	// Register our message handler
	m_pNetServer.set_open_handler(std::bind(&H5ServerNetworkImp::on_open, this, std::placeholders::_1 ));
	m_pNetServer.set_close_handler(std::bind(&H5ServerNetworkImp::on_close, this, std::placeholders::_1) );
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
			LOGFMTE("send msg to netID = %u , target is null", nConnectID);
			return false;
		}

		m_pNetServer.send(iter->second, pData, nLen, websocketpp::frame::opcode::text);
		return true;
	}
	catch ( websocketpp::exception& ec )
	{
		std::string str(pData,nLen);
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
	LOGFMTD("post close id = %u", nConnectID);
	//auto p = std::bind(&H5ServerNetworkImp::close_handler, this, nConnectID);
	//	m_ptrStrand->wrap([this, ptrSession](const asio::error_code& error) { handleAccept(error, ptrSession); })
	m_pNetServer.get_io_service().post([this,nConnectID]() 
	{
		auto piter = m_vActiveSessions.find(nConnectID);
		if ( piter == m_vActiveSessions.end())
		{
			LOGFMTE("why nconnect id = %u is null ?",nConnectID );
			return;
		}
		on_close(piter->second);
	}
	);
}

void H5ServerNetworkImp::addPacket(Packet* pPacket)
{
	std::lock_guard<std::mutex> tLock(m_PacketMutex);
	m_vRecivedPackets.push_back(pPacket);
}

void H5ServerNetworkImp::on_close( websocketpp::connection_hdl hdl )
{
	auto nConnectID = (uint32_t)(hdl.lock().get());
	LOGFMTD("begin close connectID = %u", nConnectID);
	{
		std::lock_guard<std::mutex> tLock(m_SessionMutex);
		auto iter = m_vActiveSessions.find(nConnectID);
		if (iter == m_vActiveSessions.end())
		{
			LOGFMTD("can not find  close connectID = %u", nConnectID);
			return;
		}
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

void H5ServerNetworkImp::on_open(websocketpp::connection_hdl hdl)
{
	m_SessionMutex.lock();
	m_vActiveSessions[(uint32_t)(hdl.lock().get())] = hdl;
	m_SessionMutex.unlock();

	std::string str = m_pNetServer.get_con_from_hdl(hdl)->get_host();// session->socket().remote_endpoint().address().to_string();
	LOGFMTD("a peer connected ip = %s id = %u", str.c_str(), (uint32_t)(hdl.lock().get()));
	Packet* pack = new Packet;
	pack->_brocast = false;
	pack->_packetType = _PACKET_TYPE_CONNECTED;
	pack->_connectID = (uint32_t)(hdl.lock().get());
	pack->_len = str.size();
	memset(pack->_orgdata, 0, sizeof(pack->_orgdata));
	memcpy_s(pack->_orgdata, sizeof(pack->_orgdata), str.c_str(), pack->_len);
	addPacket(pack);
}

void H5ServerNetworkImp::on_message(websocketpp::connection_hdl hdl, message_ptr msg)
{
	auto& pData = msg->get_payload();
	if ( pData.size()> _MSG_BUF_LEN)
	{
		LOGFMTE("too big buffer from connect id = %u", (uint32_t)hdl.lock().get());
		return;
	}
	Packet* pack = new Packet;
	pack->_brocast = false;
	pack->_packetType = _PACKET_TYPE_MSG;
	pack->_connectID = (uint32_t)hdl.lock().get();
	pack->_len = pData.size();
	if (pack->_len > sizeof(pack->_orgdata))
	{
		delete pack;
		pack = nullptr;
		LOGFMTE("too big recve size = %u", pData.size());
		return;
	}
	memcpy_s(pack->_orgdata, sizeof(pack->_orgdata), pData.c_str(), pack->_len);
	addPacket(pack);
}