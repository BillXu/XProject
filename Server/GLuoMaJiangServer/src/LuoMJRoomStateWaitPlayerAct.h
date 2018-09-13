#pragma once
#include "MJRoomStateWaitPlayerAct.h"
#include "LuoMJRoom.h"
class LuoMJRoomStateWaitPlayerAct
	:public MJRoomStateWaitPlayerAct
{
public:
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)override
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		if (jsTranData["idx"].isNull() == false && jsTranData["idx"].isUInt())
		{
			m_nIdx = jsTranData["idx"].asUInt();
			auto pRoom = (LuoMJRoom*)getRoom();
			pRoom->onWaitPlayerAct(m_nIdx, m_isCanPass);
			if (m_isCanPass) {
				setStateDuringTime(pRoom->isWaitPlayerActForever() ? 100000000 : eTime_WaitPlayerAct);
			}
			else {
				auto pPlayer = pRoom->getPlayerByIdx(m_nIdx);
				if (pPlayer) {
					if (pPlayer->haveState(eRoomPeer_AlreadyHu)) {
						setStateDuringTime(0);
					}
					else {
						setStateDuringTime(pRoom->isWaitPlayerActForever() ? 100000000 : eTime_WaitPlayerAct);
					}
				}
				else {
					Assert(0, "invalid argument");
					setStateDuringTime(0);
				}
			}
			return;
		}
		Assert(0, "invalid argument");
	}

	void onStateTimeUp()override
	{
		if (m_isCanPass)
		{
			m_isCanPass = false;
			auto pRoom = (LuoMJRoom*)getRoom();
			auto pPlayer = pRoom->getPlayerByIdx(m_nIdx);
			if (pPlayer) {
				if (pPlayer->haveState(eRoomPeer_AlreadyHu)) {
					setStateDuringTime(0);
				}
				else {
					setStateDuringTime(pRoom->isWaitPlayerActForever() ? 100000000 : eTime_WaitPlayerAct);
				}
			}
			else {
				Assert(0, "invalid argument");
				setStateDuringTime(0);
			}
			return;
		}
		auto nCard = ((IMJRoom*)getRoom())->getAutoChuCardWhenWaitActTimeout(m_nIdx);
		LOGFMTE("wait time out , auto chu card = %u idx = %u", nCard, m_nIdx);
		Json::Value jsTran;
		jsTran["idx"] = m_nIdx;
		jsTran["act"] = eMJAct_Chu;
		jsTran["card"] = nCard;
		getRoom()->goToState(eRoomState_DoPlayerAct, &jsTran);
	}

	void update(float fDeta)override
	{
		if (getWaitTime() > 15.0f) {
			auto pPlayer = (LuoMJPlayer*)getRoom()->getPlayerByIdx(m_nIdx);
			pPlayer->addExtraTime(fDeta);
		}
		MJRoomStateWaitPlayerAct::update(fDeta);
	}

	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override {
		if (MSG_REQ_ACT_LIST == nMsgType)
		{
			auto pPlayer = getRoom()->getPlayerBySessionID(nSessionID);
			if (pPlayer == nullptr)
			{
				LOGFMTE("you are not in room  why req act list");
				return false;
			}

			if (m_nIdx != pPlayer->getIdx())
			{
				LOGFMTD("you are not cur act player , so omit you message");
				return false;
			}

			if (m_isCanPass)
			{
				((IMJRoom*)getRoom())->onWaitPlayerAct(m_nIdx, m_isCanPass);
			}
			return true;
		}

		/*if (MSG_PLAYER_ACT == nMsgType)
		{
			auto pPlayer = getRoom()->getPlayerBySessionID(nSessionID);

			if (pPlayer == nullptr)
			{
				LOGFMTE("you are not in room  why req act list");
				return false;
			}

			auto actType = prealMsg["actType"].asUInt();
			if (eMJAct_Pass == actType)
			{
				if (pPlayer->haveState(eRoomPeer_AlreadyHu)) {
					setStateDuringTime(0);
				}
				else {
					auto pRoom = (LuoMJRoom*)getRoom();
					setStateDuringTime(pRoom->isWaitPlayerActForever() ? 100000000 : eTime_WaitPlayerAct);
				}
				return true;
			}
		}*/

		if (MSG_PLAYER_ACT != nMsgType)
		{
			return false;
		}

		auto actType = prealMsg["actType"].asUInt();
		auto nCard = prealMsg["card"].asUInt();
		auto pPlayer = (LuoMJPlayer*)getRoom()->getPlayerBySessionID(nSessionID);
		uint8_t nRet = 0;
		do
		{
			if (pPlayer == nullptr)
			{
				LOGFMTE("you are not in this room how to set ready ? session id = %u", nSessionID);
				nRet = 4;
				break;
			}

			if (m_nIdx != pPlayer->getIdx())
			{
				nRet = 1;
				break;
			}

			auto pMJCard = (LuoMJPlayerCard*)pPlayer->getPlayerCard();
			switch (actType)
			{
			case eMJAct_Chu:
			{
				if (!pMJCard->isHaveCard(nCard))
				{
					nRet = 3;
				}
			}
			break;
			case eMJAct_AnGang:
			{
				if (!pMJCard->canAnGangWithCard(nCard))
				{
					nRet = 3;
				}
			}
			break;
			case eMJAct_BuGang:
			case eMJAct_BuGang_Declare:
			{
				if (!pMJCard->canBuGangWithCard(nCard))
				{
					nRet = 3;
				}
			}
			break;
			case eMJAct_Cyclone:
			{
				if (!pMJCard->canCycloneWithCard(nCard))
				{
					nRet = 3;
				}
			}
			break;
			case eMJAct_Hu:
			{
				uint8_t nJiang = 0;
				if (!pMJCard->isHoldCardCanHu(nJiang))
				{
					nRet = 3;
				}
				nCard = pMJCard->getNewestFetchedCard();
			}
			break;
			case eMJAct_Pass:
				break;
			default:
				nRet = 2;
				break;
			}
		} while (0);

		if (nRet)
		{
			Json::Value jsRet;
			jsRet["ret"] = nRet;
			getRoom()->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			return true;
		}

		if (eMJAct_Pass == actType)
		{
			setStateDuringTime(100000000);
			return true;
		}

		// do transfer 
		Json::Value jsTran;
		jsTran["idx"] = m_nIdx;
		jsTran["act"] = actType;
		jsTran["card"] = nCard;
		jsTran["invokeIdx"] = m_nIdx;
		if (eMJAct_BuGang_Declare == actType || eMJAct_BuGang == actType)
		{
			pPlayer->signFlag(IMJPlayer::eMJActFlag_DeclBuGang);
			if (((IMJRoom*)getRoom())->isAnyPlayerRobotGang(m_nIdx, nCard))
			{
				getRoom()->goToState(eRoomState_AskForRobotGang, &jsTran);
				return true;
			}
		}
		getRoom()->goToState(eRoomState_DoPlayerAct, &jsTran);
		return true;

		//return MJRoomStateWaitPlayerAct::onMsg(prealMsg, nMsgType, eSenderPort, nSessionID);
	}
};