#pragma once
#include "IGameRoomState.h"
#include "NNRoom.h"
class NNRoomStateLRBDistributeFinalCard
	:public IGameRoomState
{
public:
	uint32_t getStateID() { return eRoomState_DistributeFinalCard; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		auto pRoom = (NNRoom*)getRoom();
		pRoom->doDistributeCard(1);
		setStateDuringTime(eTime_ExeGameStart);
	}

	void onStateTimeUp()
	{
		getRoom()->goToState(eRoomState_CaculateNiu);
	}
};