#pragma once
#include "IGameRoomState.h"
#include "BJRoom.h"
class BJRoomStateGameEnd
	:public IGameRoomState
{
public:
	uint32_t getStateID() { return eRoomState_GameEnd; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		getRoom()->onGameEnd();
		setStateDuringTime(3);
	}

	void onStateTimeUp()
	{
		getRoom()->onGameDidEnd();
		Json::Value jsValue;
		getRoom()->goToState(eRoomSate_WaitReady, &jsValue);
	}
};