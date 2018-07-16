#include "MJRoomManager.h"
#include "MJPrivateRoom.h"
IGameRoom* MJRoomManager::createRoom(uint8_t nGameType)
{
	return new MJPrivateRoom();
}

uint16_t MJRoomManager::getDiamondNeed(uint8_t nGameType, uint8_t nLevel, ePayRoomCardType payType, uint16_t nSeatCnt)
{
	return 0;
}