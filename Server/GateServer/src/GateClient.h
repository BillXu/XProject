#pragma once
#include "ServerNetwork.h"
#include "Timer.h"
struct stGateClient
{
public:
	typedef std::function<void(stGateClient*pGate,bool isWaitReconnected ) >  pfnGateClientCallBack;
public:
	stGateClient();
	~stGateClient();
	void init( unsigned int nSessionID , CONNECT_ID nNetWorkID,const char* IpInfo );
	void onReconnected( stGateClient* pBeReconnected );
	void bindUID( uint32_t nUID );
	uint32_t getBindUID();
	uint32_t getSessionID();
	bool isVerifyed();
	void doVerifyed();
	CONNECT_ID getNetworkID();
	const char* getIP();
	void doRecivedHeatBet();
	void setCloseCallBack(pfnGateClientCallBack pFun );
	void setSendHeatBeatCallBack( pfnGateClientCallBack pFun );
	void startWaitReconnect();
	void beReconnected();
	void reset();
	bool isWaitingReconnect() { return m_isWaitingReconnect; }
	void delayClose( float fDelay = 0.0001f );
protected:
	pfnGateClientCallBack m_lpfCloseCallBack;
	pfnGateClientCallBack m_lpfSendHeatBeatCallBack;
	uint32_t m_nSessionId ;
	CONNECT_ID m_nNetWorkID ;
	bool m_isVerifyed;
	uint32_t m_nUserUID;
	std::string m_strIPAddress ;

	bool m_isRecievedHeatBet;
	CTimer m_tCheckVerify;
	CTimer m_tCheckHeatBet;
	CTimer m_tWaitReconnect;
	CTimer m_tDelayClose;
	bool m_isWaitingReconnect;
};