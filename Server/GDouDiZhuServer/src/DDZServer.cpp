#include "DDZServer.h"
#include "MessageDefine.h"
#include "DDZRoomManager.h"
bool DDZServerApp::init(Json::Value& jsSvrCfg)
{
	IServerApp::init(jsSvrCfg);
	installModule(eMod_RoomMgr);
	return true ;
}

uint16_t DDZServerApp::getLocalSvrMsgPortType()
{
	return ID_MSG_PORT_DOU_DI_ZHU;
}

IGlobalModule* DDZServerApp::createModule(uint16_t eModuleType)
{
	auto p = IServerApp::createModule(eModuleType);
	if (p)
	{
		return p;
	}

	if (eMod_RoomMgr == eModuleType)
	{
		p = new DDZRoomManager();
	}
	return p;
}

