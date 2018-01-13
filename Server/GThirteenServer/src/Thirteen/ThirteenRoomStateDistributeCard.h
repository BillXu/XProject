#pragma once
#include "IGameRoomState.h"
class ThirteenRoomStateDistributeCard
	:public IGameRoomState
{
public:
	uint32_t getStateID() { return eRoomState_DistributeCard; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		auto pRoom = (ThirteenRoom*)getRoom();
		pRoom->doDistributeCard(MAX_HOLD_CARD_COUNT);
		setStateDuringTime(eTime_ExeGameStart);
	}

	void onStateTimeUp()
	{
		getRoom()->goToState(eRoomState_WaitPlayerAct);
	}
};