#pragma once
#include "IGameRoomState.h"
class GoldenRoomStateDistributeCard
	:public IGameRoomState
{
public:
	uint32_t getStateID() { return eRoomState_DistributeCard; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		auto pRoom = (GoldenRoom*)getRoom();
		pRoom->doDistributeCard(3);
		setStateDuringTime(eTime_ExeGameStart);
	}

	void onStateTimeUp()
	{
		Json::Value jsValue;
		jsValue["idx"] = ((GoldenRoom*)getRoom())->getBankerIdx();
		getRoom()->goToState(eRoomState_WaitPlayerAct, &jsValue);
	}
};