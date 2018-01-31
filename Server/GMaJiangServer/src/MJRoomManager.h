#pragma once
#include "IGameRoomManager.h"
class MJRoomManager
	:public IGameRoomManager
{
public:
	IGameRoom* createRoom(uint8_t nGameType)override;
	uint32_t getDiamondNeed(uint8_t nGameType, uint8_t nLevel, ePayRoomCardType payType )override;
};