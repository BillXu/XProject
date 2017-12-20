#pragma once
#include "IGameRoomState.h"
#include "NNRoom.h"
class NNRoomStateDecideBanker
	:public IGameRoomState
{
public:
	uint32_t getStateID() { return eRoomState_DecideBanker; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		auto pRoom = (NNRoom*)getRoom();
		auto nCandianateCnt = pRoom->doProduceNewBanker();
		float fT = nCandianateCnt * 0.5;
		setStateDuringTime(((uint8_t)fT) > 2 ? 2 : fT );
	}

	void onStateTimeUp()
	{
		Json::Value jsValue;
		getRoom()->goToState(eRoomState_DoBet, &jsValue);
	}

};