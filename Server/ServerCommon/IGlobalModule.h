#pragma once
#include "MessageDefine.h"
#include "json/json.h"
class IServerApp ;
class IGlobalModule
{
public:
	enum 
	{
		INVALID_MODULE_TYPE = (uint16_t)-1,
	};

public:
	IGlobalModule(){ m_fTicket = 300; m_app = nullptr ; m_nModuleType = INVALID_MODULE_TYPE ; }
	virtual ~IGlobalModule(){}
	IServerApp* getSvrApp(){ return m_app; }
	void setModuleType( uint8_t nModuleType ){ m_nModuleType = nModuleType ; }
	uint16_t getModuleType(){ return m_nModuleType ; };
	bool sendMsg(stMsg* pBuffer, uint16_t nLen, uint32_t nSenderUID);
	bool sendMsg(Json::Value& recvValue, uint16_t nMsgID, uint32_t nSenderUID, uint32_t nTargetID = 0, uint8_t nTargetPort = ID_MSG_PORT_CLIENT);
	virtual void init( IServerApp* svrApp ) { m_app = svrApp ; m_fTicket = getTimeSave();}
	virtual void onExit(){ onTimeSave() ;}
	virtual bool onMsg(stMsg* prealMsg , eMsgPort eSenderPort , uint32_t nSenderID ){ return false ;}
	virtual bool onMsg(Json::Value& prealMsg ,uint16_t nMsgType, eMsgPort eSenderPort , uint32_t nSenderID , uint32_t nTargetID ){ return false ;}
	virtual bool onAsyncRequest(uint16_t nRequestType , const Json::Value& jsReqContent, Json::Value& jsResult ){ return false ;};
	virtual bool onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID, uint16_t nTargetID) { return false; }
	virtual void update(float fDeta )
	{
		m_fTicket -= fDeta ;
		if ( m_fTicket < 0 )
		{
			m_fTicket = getTimeSave();
			onTimeSave() ;
		}
	}
	virtual void onTimeSave(){}
	virtual void onConnectedSvr( bool isReconnected ){}
	virtual float getTimeSave(){ return 650; }
	virtual bool onOtherSvrShutDown(eMsgPort nSvrPort, uint16_t nSvrIdx, uint16_t nSvrMaxCnt) { return false; }
public:
	friend IServerApp ;
private:
	uint16_t m_nModuleType ;
	IServerApp* m_app ;
	float m_fTicket ;
};