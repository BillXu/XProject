#pragma once
#include "MJRoomStateStartGame.h"
class SCMJRoomStateStartGame
	:public MJRoomStateStartGame
{
public:
	void onStateTimeUp()
	{
		getRoom()->goToState(eRoomState_WaitExchangeCards);
	}
};