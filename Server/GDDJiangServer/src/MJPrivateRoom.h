#pragma once 
#include "IPrivateRoom.h"
class MJPrivateRoom
	:public IPrivateRoom
{
 
public:
	GameRoom* doCreatRealRoom()override;
	void doSendRoomGameOverInfoToClient(bool isDismissed)override;
	bool canStartGame(IGameRoom* pRoom)override;
	void onStartGame(IGameRoom* pRoom)override;
	uint8_t checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)override;
};