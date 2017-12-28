#pragma once
#include "IGameRoomState.h"
#include "SCMJRoom.h"
class SCMJRoomStateExchange3Cards
	:public IGameRoomState
{
public:
	uint32_t getStateID()final { return eRoomState_WaitExchangeCards; }

	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		m_mExChangeCards.clear();
		m_bHasExchanged = false;
		auto pRoom = (SCMJRoom*)getRoom();
		if (pRoom->onWaitPlayerExchangeCards()) {
			setStateDuringTime(pRoom->isWaitPlayerActForever() ? 100000000 : eTime_WaitChoseExchangeCard);
		}
		else {
			m_bHasExchanged = true;
			setStateDuringTime(0);
		}
	}

	void update(float fDeta)override
	{
		IGameRoomState::update(fDeta);
		auto pRoom = (SCMJRoom*)getRoom();
		if (m_mExChangeCards.size() >= pRoom->getSeatCnt()) {
			pRoom->onPlayerExchangeCards(m_mExChangeCards);
			m_bHasExchanged = true;
			setStateDuringTime(eTime_DoExchangeCard);
		}
	}

	void onStateTimeUp() override
	{
		auto pRoom = (SCMJRoom*)getRoom();
		if (m_bHasExchanged) {
			pRoom->goToState(eRoomState_WaitDecideQue);
		}
		else {
			pRoom->onAutoDecidePlayerExchangeCards(m_mExChangeCards);
			setStateDuringTime(eTime_WaitChoseExchangeCard);
		}
	}

	bool onMsg(Json::Value& jsmsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID) override
	{
		if (MSG_REQ_ACT_LIST == nMsgType)
		{
			auto pPlayer = getRoom()->getPlayerBySessionID(nSessionID);
			if (pPlayer == nullptr)
			{
				LOGFMTE("you are not in room  why req act list");
				return false;
			}

			if (m_bHasExchanged) {
				return true;
			}

			if (m_mExChangeCards.count(pPlayer->getIdx())) {
				Json::Value jsMsg, jsCards;
				jsMsg["state"] = 1;
				for (auto& ref : m_mExChangeCards[pPlayer->getIdx()]) {
					jsCards[jsCards.size()] = ref;
				}
				jsMsg["cards"] = jsCards;
				getRoom()->sendMsgToPlayer(jsMsg, MSG_ROOM_SCMJ_PLAYER_EXCHANGE_CARDS, pPlayer->getSessionID());
			}
			else {
				Json::Value jsMsg;
				jsMsg["state"] = 0;
				getRoom()->sendMsgToPlayer(jsMsg, MSG_ROOM_SCMJ_PLAYER_EXCHANGE_CARDS, pPlayer->getSessionID());
			}
			return true;
		}

		if (MSG_ROOM_SCMJ_PLAYER_EXCHANGE_CARDS == nMsgType) {
			auto pPlayer = getRoom()->getPlayerBySessionID(nSessionID);
			if (pPlayer == nullptr)
			{
				LOGFMTE("you are not in room  why req act list");
				return false;
			}

			if (m_bHasExchanged) {
				return true;
			}

			if (jsmsg["cards"].isNull() || jsmsg["cards"].isArray() == false) {
				return true;
			}
			std::vector<uint8_t> vCards;
			for (auto& ref : jsmsg["cards"]) {
				if (ref.isUInt()) {
					vCards.push_back(ref.asUInt());
				}
				else {
					return true;
				}
			}
			auto pRoom = (SCMJRoom*)getRoom();
			pRoom->onPlayerDecideExchangeCards(pPlayer->getIdx(), vCards, m_mExChangeCards);
			return true;
		}

		return false;
	}

protected:
	std::map<uint8_t, std::vector<uint8_t>> m_mExChangeCards;
	bool m_bHasExchanged;
};