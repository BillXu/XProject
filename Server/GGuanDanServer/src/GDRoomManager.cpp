#include "GDRoomManager.h"
#include "GDPrivateRoom.h"
IGameRoom* GDRoomManager::createRoom(uint8_t nGameType)
{
	if (eGame_GuanDan == nGameType)
	{
		return new GDPrivateRoom();
	}
	LOGFMTE("unknown game type = %u , can not create private room", nGameType );
	return nullptr;
}