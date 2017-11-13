#include "DDZRoomManager.h"
#include "DDZPrivateRoom.h"
IGameRoom* DDZRoomManager::createRoom(uint8_t nGameType)
{
	if ( eGame_CYDouDiZhu == nGameType || eGame_JJDouDiZhu == nGameType )
	{
		return new DDZPrivateRoom();
	}
	LOGFMTE("unknown game type = %u , can not create private room", nGameType );
	return nullptr;
}

uint8_t DDZRoomManager::getDiamondNeed(uint8_t nGameType, uint8_t nLevel, ePayRoomCardType payType )
{
#ifdef _DEBUG
	return 0;
#endif // _DEBUG

	if (isCreateRoomFree())
	{
		return 0;
	}

	if (nLevel >= 3)
	{
		LOGFMTE( "invalid room level for game = %u , level = %u",nGameType,nLevel );
		nLevel = 2;
	}

	// is aa true ;
	if (ePayType_AA == payType)
	{
		uint8_t vAA[] = { 1 , 2 , 3 };
		return vAA[nLevel];
	}

	// 6,1 . 12.2 , 18. 3
	uint8_t vFangZhu[] = { 6 , 12 , 18 };
	return vFangZhu[nLevel];
}