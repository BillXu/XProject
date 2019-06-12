#pragma once
#include "MJRoomStateStartGame.h"
class SDMJRoomStateStartGame
	:public MJRoomStateStartGame
{
public:
	void onStateTimeUp()override
	{
		Json::Value jsValue;
		getRoom()->goToState(eRoomState_AutoBuHua);
	}
};