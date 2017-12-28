#pragma once
#include "IGameRoomState.h"

class GoldenRoomStateWaitPlayerAct
	:public IGameRoomState
{
public:
	uint32_t getStateID() { return eRoomState_WaitPlayerAct; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData) override
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		auto pRoom = (GoldenRoom*)getRoom();
		m_isCanPass = false;
		m_isMove = false;
		if (jsTranData["idx"].isNull() == false && jsTranData["idx"].isUInt())
		{
			m_nCurMoveIdx = jsTranData["idx"].asUInt();
			if (pRoom->onWaitPlayerAct(m_nCurMoveIdx, m_isCanPass)) {
				setStateDuringTime(pRoom->isWaitPlayerActForever() ? 100000000 : eTime_GoldenChoseAct);
			}
			else {
				setStateDuringTime(1);
			}
			return;
		}
		assert(0 && "invalid argument");
	}

	void update(float fDeta)override
	{
		IGameRoomState::update(fDeta);
		auto pRoom = (GoldenRoom*)getRoom();
		if (pRoom->isGameOver() || pRoom->isPlayerCanAct(m_nCurMoveIdx) == false)
		{
			setStateDuringTime(0);
		}
	}

	void onStateTimeUp() override
	{
		auto pRoom = (GoldenRoom*)getRoom();
		if (pRoom->isGameOver()) {
			pRoom->goToState(eRoomState_GameEnd);
		}
		else {
			if (m_isMove == false && pRoom->isPlayerCanAct(m_nCurMoveIdx)) {
				if (m_isCanPass) {
					pRoom->onPlayerPass(m_nCurMoveIdx);
				}
				else {
					pRoom->onPlayerCall(m_nCurMoveIdx);
				}
			}
			
			Json::Value jsValue;
			jsValue["idx"] = pRoom->getNextMoveIdx(m_nCurMoveIdx);
			enterState(pRoom, jsValue);
		}
	}

	bool onMsg(Json::Value& jsmsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID) override
	{
		if (MSG_ROOM_GOLDEN_GAME_WAIT_ACT == nMsgType) {
			auto pRoom = (GoldenRoom*)getRoom();
			auto pPlayer = pRoom->getPlayerBySessionID(nSessionID);
			Json::Value jsRet;
			if (pPlayer == nullptr)
			{
				jsRet["ret"] = 1;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("you are not in this room how to request acts list ? session id = %u", nSessionID);
				return true;
			}
			if (pPlayer->getIdx() != m_nCurMoveIdx) {
				jsRet["ret"] = 2;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("you are not current move player how to request acts list ? idx = %u", pPlayer->getIdx());
				return true;
			}
			if (pRoom->onWaitPlayerAct(pPlayer->getIdx(), m_isCanPass)) {
				//ÏÈ²»×ö
			}
			else {
				setStateDuringTime(0);
			}
			return true;
		}

		/*if (MSG_ROOM_GOLDEN_GAME_PASS == nMsgType) {
			auto pRoom = (GoldenRoom*)getRoom();
			auto pPlayer = pRoom->getPlayerBySessionID(nSessionID);
			Json::Value jsRet;
			if (pPlayer == nullptr)
			{
				jsRet["ret"] = 1;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("you are not in this room how to pass ? session id = %u", nSessionID);
				return true;
			}
			if (pPlayer->getIdx() != m_nCurMoveIdx) {
				jsRet["ret"] = 2;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("you are not current move player how to pass ? idx = %u", pPlayer->getIdx());
				return true;
			}
			if (pRoom->onPlayerPass(pPlayer->getIdx())) {
				m_isMove = true;
				setStateDuringTime(0);
			}
			return true;
		}*/
		
		if (MSG_ROOM_GOLDEN_GAME_CALL == nMsgType) {
			auto pRoom = (GoldenRoom*)getRoom();
			auto pPlayer = pRoom->getPlayerBySessionID(nSessionID);
			Json::Value jsRet;
			if (pPlayer == nullptr)
			{
				jsRet["ret"] = 1;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("you are not in this room how to call ? session id = %u", nSessionID);
				return true;
			}
			if (pPlayer->getIdx() != m_nCurMoveIdx) {
				jsRet["ret"] = 2;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("you are not current move player how to call ? idx = %u", pPlayer->getIdx());
				return true;
			}
			uint16_t nCoin = jsmsg["coin"].asUInt();
			bool flag = false;
			if (nCoin) {
				flag = pRoom->onPlayerAddCall(pPlayer->getIdx(), nCoin);
			}
			else {
				flag = pRoom->onPlayerCall(pPlayer->getIdx());
			}
			if (flag) {
				m_isMove = true;
				setStateDuringTime(0);
			}
			return true;
		}

		if (MSG_ROOM_GOLDEN_GAME_PK == nMsgType) {
			auto pRoom = (GoldenRoom*)getRoom();
			auto pPlayer = pRoom->getPlayerBySessionID(nSessionID);
			Json::Value jsRet;
			if (pPlayer == nullptr)
			{
				jsRet["ret"] = 1;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("you are not in this room how to PK ? session id = %u", nSessionID);
				return true;
			}
			if (pPlayer->getIdx() != m_nCurMoveIdx) {
				jsRet["ret"] = 2;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("you are not current move player how to PK ? idx = %u", pPlayer->getIdx());
				return true;
			}
			uint8_t pkWith = jsmsg["withIdx"].asUInt();
			if (pRoom->onPlayerPKWith(pPlayer->getIdx(), pkWith)) {
				m_isMove = true;
				setStateDuringTime(0);
			}
			return true;
		}

		return false;
	}

	uint8_t getCurIdx() override
	{
		return m_nCurMoveIdx;
	}

protected:
	uint8_t m_nCurMoveIdx;
	bool m_isCanPass;
	bool m_isMove;

};