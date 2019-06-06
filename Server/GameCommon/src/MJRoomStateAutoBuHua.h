#pragma once
#include "IGameRoomState.h"
#include "IMJRoom.h"
class MJRoomStateAutoBuHua
	: public IGameRoomState
{
public:
	uint32_t getStateID()final { return eRoomState_AutoBuHua; }

	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		if (jsTranData["idx"].isNull() == false && jsTranData["idx"].isUInt())
		{
			m_nIdx = jsTranData["idx"].asUInt();
		}
		else {
			m_nIdx = -1;
		}
		setStateDuringTime(0);
	}

	void onStateTimeUp()override
	{
		auto pRoom = (IMJRoom*)getRoom();
		if ( ((uint8_t)-1 == m_nIdx && pRoom->isDoAutoBuHua()) || (uint8_t)-1 != m_nIdx ) {
			if (pRoom->doPlayerBuHua(m_nIdx)) {
				setStateDuringTime(0.2);
				return;
			}
		}
		
		Json::Value jsTranData;
		if ((uint8_t)-1 == m_nIdx) {
			jsTranData["idx"] = pRoom->getBankerIdx();
		}
		else {
			jsTranData["idx"] = m_nIdx;
		}
		pRoom->goToState(eRoomState_WaitPlayerAct, &jsTranData);
	}

	uint8_t getCurIdx()override {
		return m_nIdx;
	}

private:
	uint8_t m_nIdx;
};