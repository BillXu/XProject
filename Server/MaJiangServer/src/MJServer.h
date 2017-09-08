#pragma once
#include "ISeverApp.h"
#include "Singleton.h"
class CMJServerApp
	:public IServerApp
	,public CSingleton<CMJServerApp>
{
public:
	enum eModule
	{
		eMod_None = IServerApp::eDefMod_ChildDef,
		eMod_RoomMgr = eMod_None,
		eMod_Max,
	};
public:
	CMJServerApp(){}
	bool init(Json::Value& jsSvrCfg)override;
	uint16_t getLocalSvrMsgPortType() override;
	IGlobalModule* createModule(uint16_t eModuleType)override;
};