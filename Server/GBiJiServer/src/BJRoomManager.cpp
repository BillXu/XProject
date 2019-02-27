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

uint16_t BJRoomManager::getDiamondNeed(uint8_t nGameType, uint8_t nLevel, ePayRoomCardType payType, uint16_t nSeatCnt)
{
	if (isCreateRoomFree())
	{
		return 0;
	}
//#ifdef _DEBUG
//	return 0;
//#endif // _DEBUG

	if (nLevel >= 3)
	{
		LOGFMTE( "invalid room level for game = %u , level = %u",nGameType,nLevel );
		nLevel = 2;
	}

	// is aa true ;
	if ( ePayType_AA == payType)
	{
		uint8_t vAA[] = { 15 , 30 , 45 };
		return vAA[nLevel];
	}

	if (ePayType_Winer == payType)
	{
		uint8_t vAA[] = { 60 , 120 , 180 };
		return vAA[nLevel] ;
	}

	uint8_t vFangZhu[] = { 50 , 100 , 150 };
	return vFangZhu[nLevel] ;
}