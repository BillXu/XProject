#include "RoomManager.h"
#include "NNPrivateRoom.h"
IGameRoom* RoomManager::createRoom(uint8_t nGameType)
{
	if ( eGame_NiuNiu == nGameType )
	{
		return new NNPrivateRoom();
	}
	LOGFMTE("unknown game type = %u , can not create private room", nGameType );
	return nullptr;
}

uint8_t RoomManager::getDiamondNeed(uint8_t nGameType, uint8_t nLevel, bool isAA)
{
	return 0;
}