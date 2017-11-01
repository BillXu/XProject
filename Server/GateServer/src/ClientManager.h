#pragma once
#include <map>
#include <set>
#include "ServerNetwork.h"
#include "CommonDefine.h"
#include "json\json.h"
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
	bool OnMessage( Packet* pData )override;
	bool onMsg( Json::Value& jsMsg, CONNECT_ID nNetID );
	bool onMsg( stMsg* pmsg, size_t nMsgLen ,CONNECT_ID nNetID);
	void OnNewPeerConnected( CONNECT_ID nNewPeer, ConnectInfo* IpInfo)override;
	void OnPeerDisconnected( CONNECT_ID nPeerDisconnected, ConnectInfo* IpInfo )override;
	bool OnHeatBeat(CONNECT_ID nNewPeer)override;
	stGateClient* getGateClientBySessionID(uint32_t nSessionID);
	void closeAllClient();
	void onGateCloseCallBack(stGateClient* pGateClient , bool isWaitReconnect );
	void onPlayerLogout( uint32_t sessionID );
	uint32_t getClientCnt() { return m_vSessionGateClient.size(); }
protected:
	void addClientGate(stGateClient* pGateClient );
	void removeActiveClientGate(stGateClient* pGateClient );
	stGateClient* getReserverGateClient();
	stGateClient* getGateClientByNetWorkID(CONNECT_ID nNetWorkID );
	void addToResever( stGateClient* pClient );
	void sendMsgToClient( stMsg* pmsg , uint16_t nLen ,CONNECT_ID nNetWorkID);
	void sendMsgToClient( Json::Value jsMsg, uint16_t nMsgType, CONNECT_ID nNetWorkID);
	void onRegister( Json::Value& jsMsg, stGateClient* pClient );
	void onLogin( Json::Value& jsMsg, stGateClient* pClient );
protected:
	friend struct stGateClient ;
protected:
	MAP_NETWORKID_GATE_CLIENT m_vNetWorkIDGateClientIdx ;
	MAP_SESSIONID_GATE_CLIENT m_vSessionGateClient ;

	SET_GATE_CLIENT m_vGateClientReserver ;
};