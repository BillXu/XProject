#pragma once
#include "IGameRoomState.h"
#include "NNRoom.h"
class NNRoomStateStartGame
	:public IGameRoomState
{
public:
	uint32_t getStateID() { return eRoomState_StartGame; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		getRoom()->onWillStartGame();
		getRoom()->onStartGame();
		setStateDuringTime(1);
	}

	void onStateTimeUp()
	{
		getRoom()->goToState(eRoomState_DecideBanker);
	}
};