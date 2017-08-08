#pragma once
#include <list>
#include "InternalBuffer.h"
class IServerNetworkImp
{
public:
	typedef std::list<Packet*> LIST_PACKET;
public:
	virtual ~IServerNetworkImp(){}
	virtual bool init(uint16_t nPort) = 0 ;
	virtual bool sendMsg(uint32_t nConnectID, const char* pData, size_t nLen) = 0;
	virtual bool getAllPacket(LIST_PACKET& vOutPackets) = 0; // must delete out side ;
	virtual bool getFirstPacket(Packet** ppPacket) = 0; // must delete out side ;
	virtual void closePeerConnection(uint32_t nConnectID) = 0 ;
};