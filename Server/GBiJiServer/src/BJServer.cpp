#include "BJServer.h"
#include "MessageDefine.h"
#include "BJRoomManager.h"
bool BJServerApp::init(Json::Value& jsSvrCfg)
{
	IServerApp::init(jsSvrCfg);
	installModule(eMod_RoomMgr);
	return true ;
}

uint16_t BJServerApp::getLocalSvrMsgPortType()
{
	return ID_MSG_PORT_BI_JI;
}

IGlobalModule* BJServerApp::createModule(uint16_t eModuleType)
{
	auto p = IServerApp::createModule(eModuleType);
	if (p)
	{
		return p;
	}

	 if (eMod_RoomMgr == eModuleType)
	{
		p = new BJRoomManager();
	}
	return p;
}

