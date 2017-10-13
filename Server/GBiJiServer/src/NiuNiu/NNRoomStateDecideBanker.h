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
		m_nNewBankerIdx = pRoom->doProduceNewBanker();
		setStateDuringTime(eTime_ExeGameStart);
	}

	void onStateTimeUp()
	{
		Json::Value jsValue;
		getRoom()->goToState(eRoomState_DoBet, &jsValue);
	}

	uint8_t getCurIdx()override { return m_nNewBankerIdx; };
protected:
	uint8_t m_nNewBankerIdx;
};