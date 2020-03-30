#pragma once
#include "IGameRoomState.h"
#include "GDRoom.h"
#include "GDPlayer.h"
class GDRoomStateBackTribute
	:public IGameRoomState
{
public:
	uint32_t getStateID()override { return eRoomState_GD_BackTribute; }

	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		m_mDoInfo.clear();
		m_mWaitIdx.clear();
		if (jsTranData["backInfo"].isArray()) {
			for (uint8_t i = 0; i < jsTranData["backInfo"].size(); i++) {
				auto jsInfo = jsTranData["backInfo"][i];
				m_mWaitIdx[jsInfo["idx"].asUInt()] = jsInfo["target"].asUInt();
			}
			getRoom()->sendRoomMsg(jsTranData, MSG_GD_WAIT_BACKTRIBUTE);
			setStateDuringTime(eTime_WaitForever);
		}
		else {
			setStateDuringTime(0);
		}
	}

	void update(float fDeta)override {
		if (getWaitTime() > 15.0f) {
			for (auto ref : m_mWaitIdx) {
				if (m_mDoInfo.count(ref.first)) {
					continue;
				}
				auto pPlayer = (GDPlayer*)getRoom()->getPlayerByIdx(ref.first);
				pPlayer->addExtraTime(fDeta);
			}
		}
		IGameRoomState::update(fDeta);
	}

	bool onMsg(Json::Value& jsmsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)
	{
		if (MSG_GD_DO_BACKTRIBUTE != nMsgType)
		{
			return false;
		}

		auto pRoom = (GDRoom*)getRoom();
		auto pPlayer = (GDPlayer*)pRoom->getPlayerBySessionID(nSessionID);
		Json::Value js;
		if (!pPlayer)
		{
			js["ret"] = 2;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		if (jsmsg["card"].isNull())
		{
			js["ret"] = 1;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		if (m_mWaitIdx.count(pPlayer->getIdx()) == 0 ||
			m_mDoInfo.count(pPlayer->getIdx()))
		{
			js["ret"] = 3;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		uint8_t nCard = jsmsg["card"].asUInt();
		auto pCard = pPlayer->getPlayerCard();

		if (nCard == 0 || pCard->isHaveCard(nCard) == false)
		{
			js["ret"] = 4;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		auto pTargetPlayer = (GDPlayer*)pRoom->getPlayerByIdx(m_mWaitIdx.at(pPlayer->getIdx()));

		if (!pTargetPlayer)
		{
			js["ret"] = 2;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		auto pTargetCard = pTargetPlayer->getPlayerCard();
		if (pCard->onPayCard(nCard)) {
			pTargetCard->addHoldCard(nCard);

			m_mDoInfo[pPlayer->getIdx()] = nCard;

			js["ret"] = 0;
			js["idx"] = pPlayer->getIdx();
			js["target"] = pTargetPlayer->getIdx();
			js["card"] = nCard;
			pRoom->sendRoomMsg(js, nMsgType);

			// add frame 
			pRoom->addReplayFrame(GD_Frame_BackTribute, js);

			if (m_mDoInfo.size() == m_mWaitIdx.size()) {
				setStateDuringTime(0);
			}
		}
		else {
			js["ret"] = 5;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
		}

		return true;
	}

	void onStateTimeUp() {
		auto pRoom = (GDRoom*)getRoom();
		if (m_mWaitIdx.size()) {
			for (auto& ref : m_mWaitIdx) {
				if (m_mDoInfo.count(ref.first)) {
					continue;
				}

				auto pPlayer = (GDPlayer*)pRoom->getPlayerByIdx(ref.first);
				auto pTargetPlayer = (GDPlayer*)pRoom->getPlayerByIdx(ref.second);
				if (pPlayer == nullptr || pTargetPlayer == nullptr) {
					continue;
				}
				auto nCard = pPlayer->getPlayerCard()->autoGetBackTributeCard(pRoom->getDaJi());
				if (pPlayer->getPlayerCard()->onPayCard(nCard)) {
					pTargetPlayer->getPlayerCard()->addHoldCard(nCard);

					m_mDoInfo[pPlayer->getIdx()] = nCard;

					Json::Value js;
					js["ret"] = 0;
					js["idx"] = pPlayer->getIdx();
					js["target"] = pTargetPlayer->getIdx();
					js["card"] = nCard;
					pRoom->sendRoomMsg(js, MSG_GD_DO_BACKTRIBUTE);

					// add frame 
					pRoom->addReplayFrame(GD_Frame_BackTribute, js);
				}
			}
		}

		pRoom->goToState(eRoomState_DDZ_Chu);
	}

	void roomInfoVisitor(Json::Value& js)override
	{
		IGameRoomState::roomInfoVisitor(js);
		Json::Value jsWaitIdx, jsWaitIdxes;
		for (auto ref : m_mWaitIdx) {
			jsWaitIdx["idx"] = ref.first;
			jsWaitIdx["target"] = ref.second;
			jsWaitIdxes[jsWaitIdxes.size()] = jsWaitIdx;
		}
		js["curActIdx"] = jsWaitIdxes;

		Json::Value jsAlreadyRobots;
		for (auto& ref : m_mDoInfo)
		{
			Json::Value jsItem;
			jsItem["idx"] = ref.first;
			jsItem["card"] = ref.second;
			jsAlreadyRobots[jsAlreadyRobots.size()] = jsItem;
		}
		js["readyPlayers"] = jsAlreadyRobots;
	}

protected:
	std::map<uint8_t, uint8_t> m_mWaitIdx;
	std::map<uint8_t, uint8_t> m_mDoInfo;
};