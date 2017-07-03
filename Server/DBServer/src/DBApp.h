#pragma once
#include "ISeverApp.h"
#include "ServerConfig.h"
#include "DBRWModule.h"
class CDBServerApp
	:public IServerApp
{
public:
	bool init()override;
	uint16_t getLocalSvrMsgPortType(){ return ID_MSG_PORT_DB ;}
	void update(float fdeta )override;
	void onExit()override;
	DBRWModule* getDBModule(){ return &m_tDBRW; }
	bool onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID, uint16_t nTargetID)override;
protected:
	DBRWModule m_tDBRW;
	// server config 
	CSeverConfigMgr m_stSvrConfigMgr ;
};