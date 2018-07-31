#pragma once
#include "IGameRoomState.h"
#include "CommonDefine.h"
#include "RedBlackRoom.h"
class RedBlackRoomStateResult
	:public IGameRoomState
{
public:
	uint32_t getStateID() { return eRoomState_GameEnd; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		getRoom()->onGameEnd();
		setStateDuringTime(eTime_RedBlack_Result);
	}

	void onStateTimeUp()
	{
		getRoom()->onGameDidEnd();
		Json::Value jsValue;
		getRoom()->goToState(eRoomState_StartGame, &jsValue);
	}
};