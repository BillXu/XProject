#pragma once
#include "CommonDefine.h"
#include "ServerNetwork.h"
#include <map>
#include "MessageIdentifer.h"
#include <vector>
#include "Timer.h"
class SeverInfo
{
public:
	~SeverInfo();
	void reset() { m_isRecievedHeatBeat = false; nIdx = 0; nNetworkID = INVALID_CONNECT_ID; strIp = "no ip"; }
	bool IsSvrWorking() { return nNetworkID != INVALID_CONNECT_ID; }
	void setInfo(uint16_t nSvrIdx, CONNECT_ID nNetID) { nIdx = nSvrIdx; nNetworkID = nNetID; }
	uint16_t getIdx() { return nIdx; };
	CONNECT_ID getNetworkID() { return nNetworkID; }
	void setIp( const char* ip) { strIp = ip; };
	const char* getIP() { return strIp.c_str(); }
	void startWait();
	void onVerifiedSvr();
	void onReciveHeatBeat();
protected:
	bool m_isRecievedHeatBeat;
	CTimer m_tCheckWaitVerify;
	CTimer m_tCheckHeatBeat;
	std::string strIp;
	uint16_t nIdx;
	CONNECT_ID nNetworkID;
};

// server group 
class ServerGroup
{
public:
	~ServerGroup();
	void init( eMsgPort nPortType, uint16_t nMaxSvrCnt );
	uint16_t getSvrCnt();
	bool addServer(SeverInfo* pSvr, uint16_t nTargetIdx = -1 );
	bool removeServer( CONNECT_ID nSvrConnect );
	CONNECT_ID getNetworkIDByTargetID( uint32_t nMsgTargetID );
	SeverInfo* getServerByConnectID( CONNECT_ID nSvrConnect );
	void getAllNetworkIDs( std::vector<CONNECT_ID>& vAllPortIDs );
	eMsgPort getPortType();
	const char* getPortDesc();
	bool isHaveEmptySlot();
protected:
	eMsgPort m_nPortType;
	std::vector<SeverInfo*> m_vSvrs;
};
