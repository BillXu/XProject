#pragma once
#include "ServerNetwork.h"
#include "ClientManager.h"
#include "ServerConfig.h"
#include "ISeverApp.h"
class CGateServer
	:public IServerApp
{
public:
	static CGateServer* SharedGateServer();
	CGateServer();
	~CGateServer();
	bool init(Json::Value& jsSvrCfg)override;
	CServerNetwork* GetNetWorkForClients(){ return m_pNetWorkForClients ;}
	void sendMsgToClient(const char* pData , int nLength , CONNECT_ID nSendToOrExcpet ,bool bBroadcast = false );
	CGateClientMgr* getClientMgr(){ return m_pGateManager ;}
	void update(float fDeta );
	// network
	bool OnMessage(Packet* pPacket)override;
	bool onLogicMsg(stMsg* prealMsg, eMsgPort eSenderPort, uint32_t nSenderID)override;
	uint16_t getLocalSvrMsgPortType(){ return ID_MSG_PORT_GATE ; }
	uint32_t generateSessionID();
	void onExit();
	void onConnectedToSvr(bool isReconnectMode)override;
protected:
	static CGateServer* s_GateServer ;
	CServerNetwork* m_pNetWorkForClients ;
	CGateClientMgr* m_pGateManager ;

	uint16_t m_nGatePort = 0 ;
	uint32_t m_nCurMaxSessionID ;
};