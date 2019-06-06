#include "MJPrivateRoom.h"
#include "log4z.h"
#include "SCMJRoom.h"
GameRoom* MJPrivateRoom::doCreatRealRoom()
{
	return new SCMJRoom();
}

//uint8_t MJPrivateRoom::getInitRound(uint8_t nLevel)
//{
//	return 3;
//}

void MJPrivateRoom::doSendRoomGameOverInfoToClient(bool isDismissed)
{
	LOGFMTI( "game result bill %u",isDismissed );
} 