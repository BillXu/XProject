#include "MJRoomManager.h"
#include "MJPrivateRoom.h"
IGameRoom* MJRoomManager::createRoom(uint8_t nGameType)
{
	return new MJPrivateRoom();
}

uint8_t MJRoomManager::getDiamondNeed(uint8_t nGameType, uint8_t nLevel, bool isAA)
{
	return 0;
}