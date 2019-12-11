#pragma once
#include "IGameRoomState.h"
#include "CommonDefine.h"
#include "GameRoom.h"
class GDRoomStateStartGame
	:public IGameRoomState
{
public:
	uint32_t getStateID() { return eRoomState_StartGame; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		getRoom()->onWillStartGame();
		getRoom()->onStartGame();
		setStateDuringTime(0);
	}

	void onStateTimeUp()override 
	{
		getRoom()->goToState(eRoomState_GD_PayTribute);
	}
};