#pragma once
#include "NetworkDefine.h"
#include <list>
#include "asio.hpp"
#include "InternalBuffer.h"
#include <deque> 
#include <memory>
#include <mutex>
using asio::ip::tcp;  
class CSession;
class CClientNetworkImp
{
public:
	typedef std::list<Packet*> LIST_PACKET;
	typedef std::shared_ptr<CInternalBuffer> InternalBuffer_ptr;  
	typedef std::deque<InternalBuffer_ptr> BufferQueue;
	enum  
	{
		eState_None,
		eState_Connecting,
		eState_Connected,
		eState_ConnectedFailed,
		eState_Max,
	};
public:
	CClientNetworkImp();
	~CClientNetworkImp();
	bool init();
	void shutdown();
	bool connectToServer(const char* pIP, unsigned short nPort );
	bool getAllPacket(LIST_PACKET& vOutPackets ); // must delete out side ;
	bool getFirstPacket(Packet** ppPacket ); // must delete out side ;
	void addPacket(Packet* pPacket ) ;
	bool sendMsg(const char* pData , size_t nLen ) ;
private:  
	void onReivedData(uint32_t nConnectID, const char* pBuffer, size_t nLen);
private:  
	asio::io_service m_ioservice;  
	std::shared_ptr<CSession> m_ptrSession;
	std::shared_ptr<asio::io_service::work> m_ptrIoWork;
	std::mutex m_PacketMutex;
	LIST_PACKET m_vRecivedPackets ;
	std::shared_ptr<asio::io_service::strand> m_ptrStrand;

	uint32_t m_nState;
};