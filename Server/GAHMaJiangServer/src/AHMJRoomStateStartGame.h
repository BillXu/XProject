#pragma once
#include "MJRoomStateStartGame.h"
#include "log4z.h"

class AHMJRoomStateStartGame
	:public MJRoomStateStartGame
{
public:
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)override
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		m_nGuaPuCnt = 0;
		m_bNotWaitGuaPu = false;
		m_bGameIsStart = false;
		getRoom()->onWillStartGame();
		if (getRoom()->isHaveRace()) {
			getRoom()->onWaitRace();
			setStateDuringTime(eTime_WaitPlayerAct + 1);//¶îÍâ1ÃëÓÃÓÚ·ÀÖ¹ÑÓ³Ù
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
		if (MSG_REQ_ACT_LIST == nMsgType)
		{
			getRoom()->onWaitRace(pPlayer->getIdx());
			return true;
		}

		if (MSG_ROOM_CF_GUA_PU == nMsgType) {
			Json::Value jsRet;
			if (m_bGameIsStart) {
				jsRet["ret"] = 1;
			}
			else {
				auto nRace = prealMsg["race"].asUInt();
				jsRet["race"] = nRace;
				if (nRace > 0) {
					pPlayer->setRace(nRace);
				}
				m_nGuaPuCnt++;
			}
			getRoom()->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			if (m_nGuaPuCnt >= getRoom()->getSeatCnt()) {
				setStateDuringTime(0.3);
			}
			return true;
		}

		return false;
	}

protected:
	bool m_bNotWaitGuaPu;
	bool m_bGameIsStart;
	uint8_t m_nGuaPuCnt;
};