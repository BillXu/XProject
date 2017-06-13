#pragma once
#include "ServerNetwork.h"
#include "SeverInfo.h"
#include "Timer.h"
class CCenterServerApp
	:public CServerNetworkDelegate
{
public:
	static CCenterServerApp* SharedCenterServer();
	CCenterServerApp();
	~CCenterServerApp();
	bool Init();
	void RunLoop();
	void Stop();

	virtual bool OnMessage( Packet* pData );
	virtual void OnNewPeerConnected( CONNECT_ID nNewPeer, ConnectInfo* IpInfo );
	virtual void OnPeerDisconnected( CONNECT_ID nPeerDisconnected, ConnectInfo* IpInfo );
	void closeConnection( CONNECT_ID nNetID );
	void sendMsg( const char* pmsg , uint16_t nLen , CONNECT_ID nTargetID );
public:
	static const char* getServerDescByType( uint16_t eType );
	void onSvrPortDisconnect(eMsgPort nSvrPort, uint16_t nIdx , uint16_t nMaxSvrCnt );
protected:
	static CCenterServerApp* s_GateServer ;
	bool m_bRunning ;
	CServerNetwork* m_pNetwork ;
	ServerGroup m_vTargetServers[ID_MSG_PORT_MAX];
	CTimer m_tWaitSvrSetup;
	std::map<CONNECT_ID, SeverInfo*> m_vWaitVerifyServerInfo;
};