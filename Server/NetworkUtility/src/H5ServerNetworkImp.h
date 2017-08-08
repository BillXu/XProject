#pragma once
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include "IServerNetworkImp.h"
#include "websocketpp/server.hpp"
#include "websocketpp/config/asio_no_tls.hpp"
class H5ServerNetworkImp
	:public IServerNetworkImp
{
public:
	typedef websocketpp::server<websocketpp::config::asio> server;
	typedef server::message_ptr message_ptr;

	typedef std::map<uint32_t, websocketpp::connection_hdl> MAP_SESSION;
public:
	H5ServerNetworkImp();
	~H5ServerNetworkImp();
	bool init(uint16_t nPort)override;
	bool sendMsg(uint32_t nConnectID, const char* pData, size_t nLen)override;
	bool getAllPacket(LIST_PACKET& vOutPackets)override; // must delete out side ;
	bool getFirstPacket(Packet** ppPacket)override; // must delete out side ;
	void closePeerConnection(uint32_t nConnectID)override;
protected:
	void addPacket(Packet* pPacket);
	void on_close( websocketpp::connection_hdl hdl );
	void on_open( websocketpp::connection_hdl hdl );
	void on_message( websocketpp::connection_hdl hdl, message_ptr msg );
private:
	server m_pNetServer;

	std::mutex m_SessionMutex;
	MAP_SESSION m_vActiveSessions;

	std::mutex m_PacketMutex;
	LIST_PACKET m_vRecivedPackets;

	std::shared_ptr<std::thread> m_pIOThread;
};