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
	bool OnLostSever(Packet* pMsg)override;
	bool init();
	CServerNetwork* GetNetWorkForClients(){ return m_pNetWorkForClients ;}
	void sendMsgToClient(const char* pData , int nLength , CONNECT_ID nSendToOrExcpet ,bool bBroadcast = false );
	CGateClientMgr* GetClientMgr(){ return m_pGateManager ;}
	void update(float fDeta );
	// network
	bool OnMessage(Packet* pPacket)override;
	bool onLogicMsg(stMsg* prealMsg, eMsgPort eSenderPort, uint32_t nSenderID)override;
	uint16_t getLocalSvrMsgPortType(){ return ID_MSG_PORT_GATE ; }
	uint32_t generateSessionID();
	void onExit();
protected:
	static CGateServer* s_GateServer ;
	CServerNetwork* m_pNetWorkForClients ;
	CGateClientMgr* m_pGateManager ;

	CSeverConfigMgr m_stSvrConfigMgr ;
	uint32_t m_nCurMaxSessionID ;
};