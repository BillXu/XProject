#pragma once
#include "IGameRoomManager.h"
class RoomManager
	:public IGameRoomManager
{
public:
	IGameRoom* createRoom(uint8_t nGameType)override;
	uint32_t getDiamondNeed(uint8_t nGameType, uint8_t nLevel, ePayRoomCardType payType)override;
};
