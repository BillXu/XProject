#pragma once

#include "NetWorkManager.h"

#include "Timer.h"
#include "ServerConfig.h"
#include "MessageIdentifer.h"
#include "json/json.h"
struct stMsg ;
class CAsyncRequestQuene ;
class IGlobalModule ;
class IServerApp
	:CNetMessageDelegate
{
public:
	enum eDefaultModule
	{
		eDefMod_None,
		eDefMod_AsyncRequestQueu = eDefMod_None,
		eDefMod_ChildDef,
		eDefMod_Max = eDefMod_ChildDef ,
	};
public:
	IServerApp();
	virtual ~IServerApp();
	virtual bool init();
	virtual bool OnMessage( Packet* pMsg ) ;
	virtual bool OnLostSever(Packet* pMsg);
	virtual bool OnConnectStateChanged( eConnectState eSate, Packet* pMsg);
	bool run();
	void shutDown();
	bool sendMsg( stMsg* pBuffer , uint16_t nLen, uint32_t nSenderUID );
	bool sendMsg( Json::Value& recvValue, uint16_t nMsgID, uint32_t nSenderUID, uint32_t nTargetID = 0 ,uint8_t nTargetPort = ID_MSG_PORT_CLIENT );
	void stop();
	virtual bool onLogicMsg( stMsg* prealMsg , eMsgPort eSenderPort , uint32_t nSenderID );
	virtual bool onLogicMsg( Json::Value& recvValue , uint16_t nmsgType, eMsgPort eSenderPort , uint32_t nSenderID, uint32_t nTargetID );
	void responeAsyncRequest( uint8_t nTargetPort, uint32_t nReqSerialID, uint32_t nSenderID, Json::Value& jsResult );
	virtual bool onAsyncRequest(uint16_t nRequestType , const Json::Value& jsReqContent, Json::Value& jsResult );
	virtual bool onAsyncRequestDelayResp( uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort , uint32_t nSenderID , uint16_t nTargetID );
	virtual void update(float fDeta );
	virtual uint16_t getLocalSvrMsgPortType() = 0 ; // et : ID_MSG_PORT_DATA , ID_MSG_PORT_TAXAS
	virtual uint16_t getTargetSvrPortType();
	virtual void onOtherSvrShutDown(eMsgPort nSvrPort, uint16_t nSvrIdx, uint16_t nSvrMaxCnt);
	bool isConnected();
	CTimerManager* getTimerMgr(){ return m_pTimerMgr ; }
	virtual void onExit();
	virtual void onConnectedToSvr( bool isReconnectMode );
	IGlobalModule* getModuleByType( uint16_t nType );
	CAsyncRequestQuene* getAsynReqQueue();
	uint16_t getCurSvrIdx() { return m_nCurSvrIdx; }
	uint16_t getCurSvrMaxCnt() { return m_nCurSvrPortMaxCnt; }
protected:
	bool installModule( uint16_t nModuleType );
	virtual IGlobalModule* createModule( uint16_t eModuleType );
	void setConnectServerConfig(stServerConfig* pConfig );
	void doConnectToTargetSvr();
	CNetWorkMgr* getNetwork(){ return m_pNetWork ;}
private:
	bool sendMsg(const char* pBuffer, int nLen);
	bool registerModule(IGlobalModule* pModule, uint16_t eModuleType );
private:
	bool m_bRunning;
	CONNECT_ID m_nTargetSvrNetworkID ;
	CNetWorkMgr::eConnectType m_eConnectState ;
	CTimerManager* m_pTimerMgr ;
	CNetWorkMgr* m_pNetWork ;

	stServerConfig m_stConnectConfig ;
	float m_fReconnectTick ;
	bool m_isSvrFull = false;

	char m_pSendBuffer[MAX_MSG_BUFFER_LEN] ;

	std::map<uint16_t,IGlobalModule*> m_vAllModule ;

	uint32_t m_nFrameCnt;
	float m_fFrameTicket;
	float m_fOutputfpsTickt;

	uint16_t m_nCurSvrIdx;
	uint16_t m_nCurSvrPortMaxCnt;
};