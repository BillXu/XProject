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

uint16_t DDZRoomManager::getDiamondNeed(uint8_t nGameType, uint8_t nLevel, ePayRoomCardType payType, uint16_t nSeatCnt )
{
//#ifdef _DEBUG
//	return 0;
//#endif // _DEBUG

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
		uint16_t vAA[] = { 1 , 2 , 3 };
		return vAA[nLevel] * 10;
	}

	// 6,1 . 12.2 , 18. 3
	uint16_t vFangZhu[] = { 3 , 6 , 9 };
	return vFangZhu[nLevel] * 10;
}