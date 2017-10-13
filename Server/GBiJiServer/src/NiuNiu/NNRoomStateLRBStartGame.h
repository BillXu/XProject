#pragma once
#include "NNRoomStateStartGame.h"
class NNRoomStateLRBStartGame
	:public NNRoomStateStartGame
{
public:
	void onStateTimeUp()
	{
		getRoom()->goToState(eRoomState_DistributeFirstCard);
	}
};