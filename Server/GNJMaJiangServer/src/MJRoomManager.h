#pragma once
#include "IGameRoomManager.h"
class MJRoomManager
	:public IGameRoomManager
{
public:
	IGameRoom* createRoom(uint8_t nGameType)override;
};