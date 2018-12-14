#pragma once
#include "MJRoomStateStartGame.h"
#include "log4z.h"

class NCMJRoomStateStartGame
	:public MJRoomStateStartGame
{
public:
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)override
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		m_mGuaPuInfo.clear();
		m_bNotWaitGuaPu = false;
		m_bGameIsStart = false;
		getRoom()->onWillStartGame();
		if (getRoom()->isHaveRace()) {
			getRoom()->onWaitRace();
			setStateDuringTime(999999);//¶îÍâ1ÃëÓÃÓÚ·ÀÖ¹ÑÓ³Ù
		}
		else {
			m_bGameIsStart = true;
			getRoom()->onStartGame();
			m_bNotWaitGuaPu = true;
			setStateDuringTime(eTime_ExeGameStart);
		}
	}

	void onStateTimeUp()override
	{
		if (m_bNotWaitGuaPu) {
			MJRoomStateStartGame::onStateTimeUp();
		}
		else {
			m_bGameIsStart = true;
			getRoom()->onStartGame();
			m_bNotWaitGuaPu = true;
			setStateDuringTime(eTime_ExeGameStart);
		}
	}

	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override
	{
		auto pPlayer = getRoom()->getPlayerBySessionID(nSessionID);
		if (pPlayer == nullptr)
		{
			LOGFMTE("you are not in room  why req act list");
			return false;
		}

		if (nMsgType != MSG_REQ_ACT_LIST && nMsgType != MSG_PLAYER_DO_GUA_PU) {
			return false;
		}

		if (hasGuaPu(pPlayer->getIdx()) || m_bGameIsStart) {
			return true;
		}

		if (MSG_REQ_ACT_LIST == nMsgType)
		{
			getRoom()->onWaitRace(pPlayer->getIdx());
			return true;
		}

		if (MSG_PLAYER_DO_GUA_PU == nMsgType) {
			Json::Value jsRet;
			uint8_t nRet = 0;
			jsRet["idx"] = pPlayer->getIdx();
			if (m_bGameIsStart) {
				nRet = 1;
			}
			else {
				nRet = 0;
				auto nRace = prealMsg["race"].asUInt();
				jsRet["race"] = nRace;
				if (nRace > 0) {
					pPlayer->setRace(nRace);
				}
				m_mGuaPuInfo[pPlayer->getIdx()] = nRace;
			}
			jsRet["ret"] = nRet;
			if (nRet) {
				getRoom()->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			}
			else {
				getRoom()->sendRoomMsg(jsRet, nMsgType);
			}

			if (m_mGuaPuInfo.size() >= getRoom()->getSeatCnt()) {
				setStateDuringTime(0.3);
			}
			return true;
		}

		return false;
	}

protected:
	bool hasGuaPu(uint8_t nIdx) {
		return m_mGuaPuInfo.count(nIdx);
	}

protected:
	bool m_bNotWaitGuaPu;
	bool m_bGameIsStart;
	std::map<uint8_t, uint8_t> m_mGuaPuInfo;
};