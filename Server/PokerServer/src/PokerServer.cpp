#include "PokerServer.h"
#include "MessageDefine.h"
#include <ctime>
#include "log4z.h"
#include "RoomManager.h"
bool PokerServerApp::init(Json::Value& jsSvrCfg)
{
	IServerApp::init(jsSvrCfg);
	srand((unsigned int)time(0));
	installModule(eMod_RoomMgr);
	return true ;
}

uint16_t PokerServerApp::getLocalSvrMsgPortType()
{
	return ID_MSG_PORT_POKER;
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
