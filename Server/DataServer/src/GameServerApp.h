#pragma once
#include "ISeverApp.h"
#include "PlayerManager.h"
#include "ConfigManager.h"
#include "ServerConfig.h"
#include "PokerCircle.h"
#include "Singleton.h"
class DataServerApp
	:public IServerApp
	, public CSingleton<DataServerApp>
{
public:
	enum eInstallModule
	{
		eMod_None = IServerApp::eDefMod_ChildDef,
		eMod_PlayerMgr,
		eMod_Max,
	};
public:
	~DataServerApp();
	bool init();
	CPlayerManager* getPlayerMgr();
	CConfigManager* getConfigMgr(){ return m_pConfigManager ; }
protected:
	bool onLogicMsg(stMsg* prealMsg, eMsgPort eSenderPort, uint32_t nSenderID)override;
	bool onLogicMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID)override;
	bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)override;
	uint16_t getLocalSvrMsgPortType(){ return ID_MSG_PORT_DATA ; } ; // et : ID_MSG_PORT_DATA , ID_MSG_PORT_TAXAS
	IGlobalModule* createModule( uint16_t eModuleType );
protected:
	bool processPublicMsg( stMsg* prealMsg , eMsgPort eSenderPort , uint32_t nSessionID );
protected:
	CConfigManager* m_pConfigManager ;
};