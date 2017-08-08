#pragma once
#include "asio.hpp"
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include "IServerNetworkImp.h"
using asio::ip::tcp;  
class CSession;
class CServerNetworkImp
	:public IServerNetworkImp
{
public:
	typedef std::map<uint32_t,std::shared_ptr<CSession>> MAP_SESSION ;
	typedef std::shared_ptr<CSession> session_ptr ;
public:
	CServerNetworkImp();
	~CServerNetworkImp();
	bool init(uint16_t nPort )override ;
	bool sendMsg(uint32_t nConnectID , const char* pData , size_t nLen )override;
	bool getAllPacket(LIST_PACKET& vOutPackets )override; // must delete out side ;
	bool getFirstPacket(Packet** ppPacket )override; // must delete out side ;
	void closePeerConnection( uint32_t nConnectID )override;
	void handleAccept( const asio::error_code& error, session_ptr session );
protected:
	void doCloseSession(uint32_t nConnectID, bool bServerClose);
	void startAccept() ;
	void addPacket(Packet* pPacket ) ;
	void onReivedData(uint32_t nConnectID , const char* pBuffer , size_t nLen );
	session_ptr getSessionByConnectID(uint32_t nConnectID);
protected:
	friend class CSession; 
private:  
	asio::io_service m_ioService;  
	tcp::acceptor* m_acceptor; 
  
	std::mutex m_SessionMutex;
	MAP_SESSION m_vActiveSessions ;

	std::mutex m_PacketMutex;
	LIST_PACKET m_vRecivedPackets ;

	std::shared_ptr<std::thread> m_pIOThread , m_pIOThread2;

	std::shared_ptr<asio::io_service::strand> m_ptrStrand;
};