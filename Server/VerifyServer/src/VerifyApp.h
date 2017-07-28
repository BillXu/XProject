#pragma once
#include "ISeverApp.h"
#include "TaskPoolModule.h"
class CVerifyApp
	:public IServerApp
{
public:
	enum eMod
	{
		eMod_Pool = eDefMod_ChildDef ,
		eMod_Http,
	};
public:
	bool init(Json::Value& jsSvrCfg)override;
	uint16_t getLocalSvrMsgPortType()override;
	CTaskPoolModule* getTaskPoolModule();
	IGlobalModule* createModule( uint16_t eModuleType )override ;
	const char* getWebchatNotifyUrl();
	Json::Value& getDBCfg();
protected:
	std::string m_strWebchatNotifyUrl;
	Json::Value m_jsDBCfg;
};