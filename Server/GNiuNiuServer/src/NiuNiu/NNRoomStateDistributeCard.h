#pragma once
#include "IGameRoomState.h"
#include "NNRoom.h"
class NNRoomStateDistributeCard
	:public IGameRoomState
{
public:
	uint32_t getStateID() { return eRoomState_DistributeCard; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		auto pRoom = (NNRoom*)getRoom();
		pRoom->doDistributeCard(5);
		setStateDuringTime(eTime_ExeGameStart);
	}

	void onStateTimeUp()
	{
		Json::Value jsValue;
		getRoom()->goToState(eRoomState_CaculateNiu, &jsValue);
	}
};