#pragma once
#include "IGlobalModule.h"
#include <json/json.h>
struct stMsg;
class IGameRoom;
class IGameRoomManager
	:public IGlobalModule
{
public:
	virtual ~IGameRoomManager(){}
	virtual IGameRoom* getRoomByID(uint32_t nRoomID) = 0;
	virtual void sendMsg(stMsg* pmsg, uint32_t nLen, uint32_t nSessionID, uint32_t nSenderID ) = 0;
	virtual void sendMsg(Json::Value& jsContent, uint16_t nMsgType, uint32_t nSessionID, uint32_t nSenderID, eMsgPort ePort = ID_MSG_PORT_CLIENT) = 0;
};