#include "MJPrivateRoom.h"
#include "log4z.h"
#include "LuoMJRoom.h"
GameRoom* MJPrivateRoom::doCreatRealRoom()
{
	return new LuoMJRoom();
}

uint8_t MJPrivateRoom::getInitRound(uint8_t nLevel)
{
	return 3;
}

void MJPrivateRoom::doSendRoomGameOverInfoToClient(bool isDismissed)
{
	LOGFMTI( "game result bill %u",isDismissed );
} 