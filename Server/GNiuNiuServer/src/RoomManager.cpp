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

uint8_t RoomManager::getDiamondNeed(uint8_t nGameType, uint8_t nLevel,ePayRoomCardType payType, uint16_t nSeatCnt)
{
	if (isCreateRoomFree())
	{
		return 0;
	}
#ifdef _DEBUG
	//return 0;
#endif // _DEBUG

	if (nLevel >= 3)
	{
		LOGFMTE( "invalid room level for game = %u , level = %u",nGameType,nLevel );
		nLevel = 2;
	}

	// is aa true ;
	if (ePayType_AA == payType)
	{
		uint8_t vAA[] = { 1 , 2 , 3 };
		return vAA[nLevel] * 10;
	}

	if ( nSeatCnt == 8 )
	{
		// 6,1 . 12.2 , 18. 3
		uint8_t vFangZhu[] = { 4 , 8 , 16 };
		return vFangZhu[nLevel] * 10;
	}
	// 6,1 . 12.2 , 18. 3
	uint8_t vFangZhu[] = { 3 , 6 , 9 };
	return vFangZhu[nLevel] * 10;
}