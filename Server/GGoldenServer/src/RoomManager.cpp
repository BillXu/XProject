#include "RoomManager.h"
#include "GoldenPrivateRoom.h"
IGameRoom* RoomManager::createRoom(uint8_t nGameType)
{
	if ( eGame_Golden == nGameType )
	{
		return new GoldenPrivateRoom();
	}
	LOGFMTE("unknown game type = %u , can not create private room", nGameType );
	return nullptr;
}

//uint16_t RoomManager::getDiamondNeed(uint8_t nGameType, uint8_t nLevel, ePayRoomCardType payType, uint16_t nSeatCnt)
//{
//	if (isCreateRoomFree())
//	{
//		return 0;
//	}
//#ifdef _DEBUG
//	//return 0;
//#endif // _DEBUG
//
//	if (nLevel >= 3)
//	{
//		LOGFMTE("invalid room level for game = %u , level = %u", nGameType, nLevel);
//		nLevel = 2;
//	}
//
//	// is aa true ;
//	if (ePayType_AA == payType)
//	{
//		uint16_t vAA[] = { 1 , 2 , 3 };
//		return vAA[nLevel] * 10 * 2;
//	}
//
//	if ( nSeatCnt == 9 )
//	{
//		uint16_t vFangZhu[] = { 90 , 180 , 270 };
//		return vFangZhu[nLevel];
//	}
//
//	if ( 12 == nSeatCnt )
//	{
//		uint16_t vFangZhu[] = { 120 , 240 , 360 };
//		return vFangZhu[nLevel];
//	}
//	// 6,1 . 12.2 , 18. 3
//	uint16_t vFangZhu[] = { 6 , 12 , 18 };
//	return vFangZhu[nLevel] * 10;
//}