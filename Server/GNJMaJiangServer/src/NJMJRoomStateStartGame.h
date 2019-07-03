#pragma once
#include "MJRoomStateStartGame.h"
class NJMJRoomStateStartGame
	:public MJRoomStateStartGame
{
public:
	void onStateTimeUp()override
	{
		Json::Value jsValue;
		getRoom()->goToState(eRoomState_AutoBuHua);
	}
};