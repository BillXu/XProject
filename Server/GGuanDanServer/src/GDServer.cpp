#include "GDServer.h"
#include "MessageDefine.h"
#include "GDRoomManager.h"
bool GDServerApp::init(Json::Value& jsSvrCfg)
{
	IServerApp::init(jsSvrCfg);
	installModule(eMod_RoomMgr);
	return true ;
}

uint16_t GDServerApp::getLocalSvrMsgPortType()
{
	return ID_MSG_PORT_GUAN_DAN;
}

IGlobalModule* GDServerApp::createModule(uint16_t eModuleType)
{
	auto p = IServerApp::createModule(eModuleType);
	if (p)
	{
		return p;
	}

	if (eMod_RoomMgr == eModuleType)
	{
		p = new GDRoomManager();
	}
	return p;
}

