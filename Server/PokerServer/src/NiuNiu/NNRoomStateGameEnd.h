#pragma once
#include "IGameRoomState.h"
#include "NNRoom.h"
class NNRoomStateGameEnd
	:public IGameRoomState
{
public:
	uint32_t getStateID() { return eRoomState_GameEnd; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		getRoom()->onGameEnd();
		setStateDuringTime(eTime_ExeGameStart);
	}

	void onStateTimeUp()
	{
		getRoom()->onGameDidEnd();
		Json::Value jsValue;
		getRoom()->goToState( eRoomSate_WaitReady, &jsValue );
	}
};