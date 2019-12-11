#pragma once
#include "IGameRoomState.h"
#include "GDRoom.h"
#include "GDPlayer.h"
#include "IPoker.h"
class GDRoomStatePayTribute
	:public IGameRoomState
{
public:
	uint32_t getStateID() { return eRoomState_GD_PayTribute; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		m_mDoPayTribute.clear();
		m_vWaitIdx.clear();

		auto pRoom = (GDRoom*)getRoom();
		if (pRoom->onWaitPayTribute(m_vWaitIdx)) {
			setStateDuringTime(eTime_WaitForever);
		}
		else {
			setStateDuringTime(0);
		}
	}

	void update(float fDeta)override {
		if (getWaitTime() > 15.0f) {
			for (auto idx : m_vWaitIdx) {
				if (m_mDoPayTribute.count(idx)) {
					continue;
				}
				auto pPlayer = (GDPlayer*)getRoom()->getPlayerByIdx(idx);
				pPlayer->addExtraTime(fDeta);
			}
		}
		IGameRoomState::update(fDeta);
	}

	bool onMsg(Json::Value& jsmsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)
	{
		if (MSG_GD_CONFIRM_PAYTRIBUTE != nMsgType)
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

		if (std::find(m_vWaitIdx.begin(), m_vWaitIdx.end(), pPlayer->getIdx()) == m_vWaitIdx.end() ||
			m_mDoPayTribute.count(pPlayer->getIdx()))
		{
			js["ret"] = 3;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		uint8_t nCard = jsmsg["card"].asUInt();

		if (nCard == 0 || pPlayer->getPlayerCard()->isCardInvalidPayTribute(nCard, pRoom->getDaJi()) == false)
		{
			js["ret"] = 4;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		m_mDoPayTribute[pPlayer->getIdx()] = nCard;

		js["ret"] = 0;
		js["idx"] = pPlayer->getIdx();
		js["card"] = nCard;
		pRoom->sendRoomMsg(js, nMsgType);

		if (m_mDoPayTribute.size() == m_vWaitIdx.size()) {
			setStateDuringTime(0);
		}

		return true;
	}

	void onStateTimeUp()
	{
		auto pRoom = (GDRoom*)getRoom();
		if (m_vWaitIdx.size()) {
			Json::Value jsMsg, jsIdx;

			for (auto idx : m_vWaitIdx) {
				jsIdx[jsIdx.size()] = idx;
				if (m_mDoPayTribute.count(idx)) {
					continue;
				}
				auto pPlayer = (GDPlayer*)pRoom->getPlayerByIdx(idx);
				auto pCard = (GDPlayerCard*)pPlayer->getPlayerCard();
				auto nCard = pCard->autoGetPayTributeCard(pRoom->getDaJi());
				if (nCard) {
					m_mDoPayTribute[idx] = nCard;
				}
				else {
					LOGFMTE("GDRoom = %u, player idx = %u auto find pay tribute card error!!", pRoom->getRoomID(), pPlayer->getIdx());
				}
			}
			std::map<uint8_t, uint8_t> mBackInfo;
			pRoom->doPayTribute(m_mDoPayTribute, mBackInfo);

			Json::Value jsInfo, jsInfos;
			for (auto ref : mBackInfo) {
				jsInfo["idx"] = ref.first;
				jsInfo["target"] = ref.second;
				jsInfos[jsInfos.size()] = jsInfo;
			}
			
			jsMsg["backInfo"] = jsInfos;
			pRoom->goToState(eRoomState_GD_BackTribute, &jsMsg);
		}
		else {
			pRoom->goToState(eRoomState_DDZ_Chu);
		}
	}

	void roomInfoVisitor(Json::Value& js)override
	{
		IGameRoomState::roomInfoVisitor(js);
		Json::Value jsWaitIdx;
		for (auto ref : m_vWaitIdx) {
			jsWaitIdx[jsWaitIdx.size()] = ref;
		}
		js["curActIdx"] = jsWaitIdx;

		Json::Value jsAlreadyRobots;
		for (auto& ref : m_mDoPayTribute)
		{
			Json::Value jsItem;
			jsItem["idx"] = ref.first;
			jsItem["card"] = ref.second;
			jsAlreadyRobots[jsAlreadyRobots.size()] = jsItem;
		}
		js["readyPlayers"] = jsAlreadyRobots;
	}

protected:
	std::vector<uint8_t> m_vWaitIdx;
	std::map<uint8_t, uint8_t> m_mDoPayTribute;
};