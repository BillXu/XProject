#pragma once
#include "DDZRoomStateWaitReady.h"
class JJDDZRoomStateWaitReadyChaoZhuangMode
	:public DDZRoomStateWaitReady
{
public:
	void update(float fDeta)override
	{
		IGameRoomState::update(fDeta);
		auto pRoom = getRoom();
		if (pRoom->canStartGame())
		{
			pRoom->goToState(eRoomState_JJ_DDZ_Chao_Zhuang);
		}
	}
};