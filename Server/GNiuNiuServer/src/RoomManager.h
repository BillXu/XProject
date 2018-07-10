#pragma once
#include "IGameRoomManager.h"
class RoomManager
	:public IGameRoomManager
{
public:
	IGameRoom* createRoom(uint8_t nGameType)override;
	uint8_t getDiamondNeed(uint8_t nGameType, uint8_t nLevel, ePayRoomCardType payType, uint16_t nSeatCnt )override;
};
