#pragma once
#include "IGameRoomState.h"
#include <cassert>
#include "FXMJRoom.h"
class FXMJRoomStateWaitPlayerGangTing
	:public IGameRoomState
{
public:
	uint32_t getStateID()final { return eRoomState_AfterGang; }

	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)override
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		m_bDone = false;
		if (jsTranData["idx"].isNull() == false && jsTranData["idx"].isUInt())
		{
			m_nIdx = jsTranData["idx"].asUInt();
			auto pRoom = (FXMJRoom*)getRoom();
			if (pRoom->onWaitPlayerGangTing(m_nIdx)) {
				setStateDuringTime(100000000);
			}
			else {
				setStateDuringTime(0);
			}
			return;
		}
		assert(0 && "invalid argument");
	}

	void onStateTimeUp()override
	{
		auto pRoom = (FXMJRoom*)getRoom();
		pRoom->onPlayerSingalMo(m_nIdx);
		Json::Value jsValue;
		jsValue["idx"] = m_nIdx;
		getRoom()->goToState(eRoomState_WaitPlayerAct, &jsValue);
	}

	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override
	{
		if (MSG_ROOM_FXMJ_DO_GANG_TING == nMsgType) {
			Json::Value jsMsg;
			auto pRoom = (FXMJRoom*)getRoom();

			if (m_bDone) {
				jsMsg["ret"] = 1;
				pRoom->sendMsgToPlayer(jsMsg, MSG_ROOM_FXMJ_DO_GANG_TING, nSessionID);
				return true;
			}
			
			if (prealMsg["ting"].isUInt() == false) {
				jsMsg["ret"] = 2;
				pRoom->sendMsgToPlayer(jsMsg, MSG_ROOM_FXMJ_DO_GANG_TING, nSessionID);
				return true;
			}
			
			auto pPlayer = pRoom->getPlayerBySessionID(nSessionID);
			if (pPlayer == nullptr || pPlayer->getIdx() != m_nIdx) {
				jsMsg["ret"] = 3;
				pRoom->sendMsgToPlayer(jsMsg, MSG_ROOM_FXMJ_DO_GANG_TING, nSessionID);
				return true;
			}

			m_bDone = true;
			uint8_t nTing = prealMsg["ting"].asUInt();
			if (nTing) {
				pRoom->onPlayerTing(m_nIdx, nTing);
			}
			setStateDuringTime(0);

			return true;
		}

		return false;
	}

	uint8_t getCurIdx()override
	{
		return m_nIdx;
	}

protected:
	uint8_t m_nIdx;
	bool m_bDone;
};