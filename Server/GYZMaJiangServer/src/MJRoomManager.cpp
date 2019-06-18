#include "MJRoomManager.h"
#include "MJPrivateRoom.h"
IGameRoom* MJRoomManager::createRoom(uint8_t nGameType)
{
	return new MJPrivateRoom();
}

//uint16_t MJRoomManager::getDiamondNeed(uint8_t nGameType, uint8_t nLevel, ePayRoomCardType payType, uint16_t nSeatCnt)
//{
//	//return 0;
//
//	if (isCreateRoomFree())
//	{
//		return 0;
//	}
//#ifdef _DEBUG
//	//return 0;
//#endif // _DEBUG
//
//	if (nLevel > 1)
//	{
//		LOGFMTE("invalid room level for game = %u , level = %u", nGameType, nLevel);
//		nLevel = 1;
//	}
//
//	// is aa true ;
//	if (ePayType_AA == payType)
//	{
//		uint16_t vAA[] = { 1 , 2 };
//		return vAA[nLevel];
//	}
//
//	//if (nSeatCnt == 8)
//	//{
//	//	// 6,1 . 12.2 , 18. 3
//	//	uint16_t vFangZhu[] = { 4 , 8 , 12 };
//	//	return vFangZhu[nLevel] * 10;
//	//}
//	// 6,1 . 12.2 , 18. 3
//	uint16_t vFangZhu[] = { 4 , 8 };
//	return vFangZhu[nLevel];
//}