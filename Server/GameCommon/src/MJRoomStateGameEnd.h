#pragma once 
#include "IGameRoomState.h"
#include "IMJRoom.h"
class MJRoomStateGameEnd
	:public IGameRoomState
{
public:
	uint32_t getStateID(){ return eRoomState_GameEnd; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		getRoom()->onGameEnd();
		setStateDuringTime(eTime_GameOver * 0.1);
	}

	void onStateTimeUp()
	{
		getRoom()->onGameDidEnd();
		getRoom()->goToState(eRoomSate_WaitReady);
	}
};




