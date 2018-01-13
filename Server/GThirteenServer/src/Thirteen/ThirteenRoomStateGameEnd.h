#pragma once
#include "IGameRoomState.h"
class ThirteenRoomStateGameEnd
	:public IGameRoomState
{
public:
	uint32_t getStateID() { return eRoomState_GameEnd; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		getRoom()->onGameEnd();
		setStateDuringTime(eTime_GameOver);
	}

	void onStateTimeUp()
	{
		getRoom()->onGameDidEnd();
		Json::Value jsValue;
		getRoom()->goToState( eRoomSate_WaitReady, &jsValue );
	}
};