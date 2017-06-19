#pragma once
#include <map>
#include <set>
#include "ServerNetwork.h"
#include "CommonDefine.h"
struct stGateClient ;
struct stMsg;
class CGateClientMgr
	:public CServerNetworkDelegate
{
public:
	typedef std::map<unsigned int ,stGateClient*> MAP_SESSIONID_GATE_CLIENT ;
	typedef std::map<CONNECT_ID, stGateClient*> MAP_NETWORKID_GATE_CLIENT ;
	typedef std::set<stGateClient*> SET_GATE_CLIENT ;
public:
	CGateClientMgr();
	~CGateClientMgr();
	virtual bool OnMessage( Packet* pData ) ;
	virtual void OnNewPeerConnected( CONNECT_ID nNewPeer, ConnectInfo* IpInfo);
	virtual void OnPeerDisconnected( CONNECT_ID nPeerDisconnected, ConnectInfo* IpInfo );
	stGateClient* getGateClientBySessionID(uint32_t nSessionID);
	void closeAllClient();
	void onGateCloseCallBack(stGateClient* pGateClient , bool isWaitReconnect );
protected:
	void addClientGate(stGateClient* pGateClient );
	void removeActiveClientGate(stGateClient* pGateClient );
	stGateClient* getReserverGateClient();
	stGateClient* GetGateClientByNetWorkID(CONNECT_ID& nNetWorkID );
	void addToResever( stGateClient* pClient );
	void sendMsgToClient( stMsg* pmsg , uint16_t nLen ,CONNECT_ID nNetWorkID);
protected:
	friend struct stGateClient ;
protected:
	MAP_NETWORKID_GATE_CLIENT m_vNetWorkIDGateClientIdx ;
	MAP_SESSIONID_GATE_CLIENT m_vSessionGateClient ;

	SET_GATE_CLIENT m_vGateClientReserver ;
};