#include "BJRoomManager.h"
#include "./BiJi/BJPrivateRoom.h"
IGameRoom* BJRoomManager::createRoom(uint8_t nGameType)
{
	if ( eGame_BiJi == nGameType )
	{
		return new BJPrivateRoom();
	}
	LOGFMTE("unknown game type = %u , can not create private room", nGameType );
	return nullptr;
}

uint8_t BJRoomManager::getDiamondNeed(uint8_t nGameType, uint8_t nLevel, ePayRoomCardType payType )
{
	if (isCreateRoomFree())
	{
		return 0;
	}
#ifdef _DEBUG
	return 0;
#endif // _DEBUG

	if (nLevel >= 3)
	{
		LOGFMTE( "invalid room level for game = %u , level = %u",nGameType,nLevel );
		nLevel = 2;
	}

	// is aa true ;
	if ( ePayType_AA == payType)
	{
		uint8_t vAA[] = { 1 , 2 , 3 };
		return vAA[nLevel] * 10;
	}

	uint8_t vFangZhu[] = { 6 , 12 , 18 };
	return vFangZhu[nLevel] * 10;
}