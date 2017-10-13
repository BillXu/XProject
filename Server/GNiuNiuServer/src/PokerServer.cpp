#include "PokerServer.h"
#include "MessageDefine.h"
#include "RoomManager.h"
bool PokerServerApp::init(Json::Value& jsSvrCfg)
{
	IServerApp::init(jsSvrCfg);
	installModule(eMod_RoomMgr);
	return true ;
}

uint16_t PokerServerApp::getLocalSvrMsgPortType()
{
	return ID_MSG_PORT_NIU_NIU;
}

IGlobalModule* PokerServerApp::createModule(uint16_t eModuleType)
{
	auto p = IServerApp::createModule(eModuleType);
	if (p)
	{
		return p;
	}

	 if (eMod_RoomMgr == eModuleType)
	{
		p = new RoomManager();
	}
	return p;
}

