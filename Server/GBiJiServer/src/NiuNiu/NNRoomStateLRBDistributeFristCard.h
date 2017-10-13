#pragma once
#include "IGameRoomState.h"
#include "NNRoom.h"
class NNRoomStateLRBDistributeFristCard
	:public IGameRoomState
{
public:
	uint32_t getStateID() { return eRoomState_DistributeFirstCard; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		auto pRoom = (NNRoom*)getRoom();
		pRoom->doDistributeCard(4);
		setStateDuringTime(eTime_ExeGameStart);
	}

	void onStateTimeUp()
	{
		getRoom()->goToState(eRoomState_RobotBanker);
	}
};