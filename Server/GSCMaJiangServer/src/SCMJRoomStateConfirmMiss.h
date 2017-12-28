#pragma once
#include "IGameRoomState.h"
#include "SCMJRoom.h"
class SCMJRoomStateConfirmMiss
	:public IGameRoomState
{
public:
	uint32_t getStateID()final { return eRoomState_WaitDecideQue; }

	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		m_bIsShowPlayerMiss = false;
		auto pRoom = (SCMJRoom*)getRoom();
		if (pRoom->onWaitPlayerConfirmMiss()) {
			setStateDuringTime(pRoom->isWaitPlayerActForever() ? 100000000 : eTime_WaitDecideQue);
		}
		else {
			m_bIsShowPlayerMiss = true;
			setStateDuringTime(0);
		}
	}

	void update(float fDeta)override
	{
		IGameRoomState::update(fDeta);
		auto pRoom = (SCMJRoom*)getRoom();
		if (pRoom->isAllPlayerConfirmMiss()) {
			m_bIsShowPlayerMiss = true;
			pRoom->onShowPlayerMiss();
			setStateDuringTime(eTime_DoDecideQue);
		}
	}

	void onStateTimeUp() override
	{
		auto pRoom = (SCMJRoom*)getRoom();
		if (m_bIsShowPlayerMiss) {
			Json::Value jsValue;
			jsValue["idx"] = pRoom->getBankerIdx();
			getRoom()->goToState(eRoomState_WaitPlayerAct, &jsValue);
		}
		else {
			pRoom->onAutoConfirmMiss();
			setStateDuringTime(eTime_WaitDecideQue);
		}
	}

	bool onMsg(Json::Value& jsmsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID) override
	{
		if (MSG_REQ_ACT_LIST == nMsgType)
		{
			auto pPlayer = (SCMJPlayer*)getRoom()->getPlayerBySessionID(nSessionID);
			if (pPlayer == nullptr)
			{
				LOGFMTE("you are not in room  why req act list");
				return false;
			}

			if (m_bIsShowPlayerMiss) {
				return true;
			}

			auto pCard = (SCMJPlayerCard*)pPlayer->getPlayerCard();
			Json::Value jsMsg;
			if (pCard->isDecideMiss()) {
				jsMsg["state"] = 1;
				jsMsg["missType"] = pCard->getMissType();
			}
			else {
				jsMsg["state"] = 0;
			}
			getRoom()->sendMsgToPlayer(jsMsg, MSG_ROOM_SCMJ_PLAYER_DECIDE_MISS, pPlayer->getSessionID());
			return true;
		}

		if (MSG_ROOM_SCMJ_PLAYER_DECIDE_MISS == nMsgType) {
			auto pPlayer = getRoom()->getPlayerBySessionID(nSessionID);
			if (pPlayer == nullptr)
			{
				LOGFMTE("you are not in room  why req act list");
				return false;
			}

			if (m_bIsShowPlayerMiss) {
				return true;
			}

			if (jsmsg["missType"].isNull() || jsmsg["missType"].isUInt() == false) {
				return false;
			}

			((SCMJRoom*)getRoom())->onPlayerConfirmMiss(pPlayer->getIdx(), jsmsg["missType"].asUInt());
			return true;
		}

		return false;
	}

protected:
	bool m_bIsShowPlayerMiss;
};