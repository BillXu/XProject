#include "MJServer.h"
#include "MessageDefine.h"
#include "MJRoomManager.h"
bool CMJServerApp::init(Json::Value& jsSvrCfg)
{
	IServerApp::init(jsSvrCfg);
	installModule(eMod_RoomMgr);
	return true;
}

uint16_t CMJServerApp::getLocalSvrMsgPortType()
{
	return ID_MSG_PORT_AHMJ;
}

IGlobalModule* CMJServerApp::createModule(uint16_t eModuleType)
{
	auto p = IServerApp::createModule(eModuleType);
	if (p)
	{
		return p;
	}

	if (eMod_RoomMgr == eModuleType)
	{
		p = new MJRoomManager();
	}
	return p;
}

