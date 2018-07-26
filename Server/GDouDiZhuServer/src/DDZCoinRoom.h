#pragma once
#include "ICoinRoom.h"
class DDZCoinRoom
	:public ICoinRoom
{
public:
	bool onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)override;
	GameRoom* doCreatRealRoom()override;
	void onGameDidEnd(IGameRoom* pRoom)override;
};