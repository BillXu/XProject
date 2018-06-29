#pragma once
#include "NNRoomStateStartGame.h"
#include "NNRoom.h"
class NNRoomStateStartGameNoNiuLeaveBanker
	:public NNRoomStateStartGame
{
public:
	void onStateTimeUp()
	{
		auto pRoom = (NNRoom*)getRoom();
		auto nBankerIdx = pRoom->getBankerIdx();
		if ((decltype(nBankerIdx))-1 == nBankerIdx)
		{
			getRoom()->goToState(eRoomState_RobotBanker);
		}
		else
		{
			getRoom()->goToState(eRoomState_DoBet);
		}
	}
};
