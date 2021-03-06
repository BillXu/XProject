#pragma once
#include "ISeverApp.h"
#include "Singleton.h"
class PokerServerApp
	:public IServerApp
	,public CSingleton<PokerServerApp>
{
public:
	enum eModule
	{
		eMod_None = IServerApp::eDefMod_ChildDef,
		eMod_RoomMgr = eMod_None,
		eMod_Max,
	};
public:
	PokerServerApp(){}
	bool init(Json::Value& jsSvrCfg)override;
	uint16_t getLocalSvrMsgPortType() override ;
	IGlobalModule* createModule(uint16_t eModuleType)override;
};