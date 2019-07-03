#pragma once
#include "MJRoomStateWaitPlayerAct.h"
#include "NJMJRoom.h"
class NJMJRoomStateWaitPlayerAct
	:public MJRoomStateWaitPlayerAct
{
public:
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)override
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		if (jsTranData["idx"].isNull() == false && jsTranData["idx"].isUInt())
		{
			m_isCanPass = false;
			m_nIdx = jsTranData["idx"].asUInt();
			auto pRoom = (NJMJRoom*)getRoom();

			if (pRoom->doPlayerBuHua(m_nIdx)) {
				pRoom->goToState(eRoomState_AutoBuHua, &jsTranData);
				return;
			}

			pRoom->onWaitPlayerAct(m_nIdx, m_isCanPass);
			if (m_isCanPass) {
				setStateDuringTime(pRoom->isWaitPlayerActForever() ? 100000000 : eTime_WaitPlayerAct);
			}
			else {
				auto pPlayer = pRoom->getPlayerByIdx(m_nIdx);
				if (pPlayer) {
					if (pPlayer->haveState(eRoomPeer_AlreadyHu) || pRoom->needChu() == false) {
						setStateDuringTime(0.5);
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

	void update(float fDeta)override
	{
		if (getWaitTime() > 15.0f) {
			auto pPlayer = (NJMJPlayer*)getRoom()->getPlayerByIdx(m_nIdx);
			pPlayer->addExtraTime(fDeta);
		}
		MJRoomStateWaitPlayerAct::update(fDeta);
	}

	void onStateTimeUp()override
	{
		auto pRoom = (NJMJRoom*)getRoom();
		if (m_isCanPass)
		{
			m_isCanPass = false;
			auto pPlayer = pRoom->getPlayerByIdx(m_nIdx);
			if (pPlayer) {
				if (pPlayer->haveState(eRoomPeer_AlreadyHu) || pRoom->needChu() == false) {
					setStateDuringTime(0.5);
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
		if (pRoom->needChu()) {
			auto nCard = pRoom->getAutoChuCardWhenWaitActTimeout(m_nIdx);
			LOGFMTE("wait time out , auto chu card = %u idx = %u", nCard, m_nIdx);
			Json::Value jsTran;
			jsTran["idx"] = m_nIdx;
			jsTran["act"] = eMJAct_Chu;
			jsTran["card"] = nCard;
			pRoom->goToState(eRoomState_DoPlayerAct, &jsTran);
		}
		else if(pRoom->isCanGoOnMoPai()){
			auto nIdx = pRoom->getNextActPlayerIdx(m_nIdx);
			Json::Value jsTran;
			jsTran["idx"] = nIdx;
			jsTran["act"] = eMJAct_Mo;
			pRoom->goToState(eRoomState_DoPlayerAct, &jsTran);
		}
		else {
			pRoom->goToState(eRoomState_GameEnd);
		}
		
	}

	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override {
		auto pRoom = (NJMJRoom*)getRoom();
		auto pPlayer = (NJMJPlayer*)pRoom->getPlayerBySessionID(nSessionID);
		if (MSG_REQ_ACT_LIST == nMsgType)
		{
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
				pRoom->onWaitPlayerAct(m_nIdx, m_isCanPass);
			}
			return true;
		}

		if (MSG_PLAYER_ACT != nMsgType)
		{
			return false;
		}
		
		auto actType = prealMsg["actType"].asUInt();
		auto nCard = prealMsg["card"].asUInt();
		
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

			auto pMJCard = (NJMJPlayerCard*)pPlayer->getPlayerCard();
			switch (actType)
			{
			case eMJAct_Chu:
			{
				if (!pMJCard->isHaveCard(nCard) || pRoom->needChu() == false || m_isCanPass)
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
			case eMJAct_Hu:
			{
				uint8_t nJiang = 0;
				if (!pMJCard->isHoldCardCanHu(nJiang, pPlayer->haveGangFlag() || pPlayer->haveFlag(IMJPlayer::eMJActFlag_BuHua)))
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
			pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			return true;
		}

		if (eMJAct_Pass == actType)
		{
			m_isCanPass = false;
			if (pRoom->needChu()) {
				setStateDuringTime(100000000);
			}
			else {
				setStateDuringTime(0);
			}
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
			if (pRoom->isAnyPlayerRobotGang(m_nIdx, nCard))
			{
				pRoom->goToState(eRoomState_AskForRobotGang, &jsTran);
				return true;
			}
		}
		pRoom->goToState(eRoomState_DoPlayerAct, &jsTran);
		return true;

		//return MJRoomStateWaitPlayerAct::onMsg(prealMsg, nMsgType, eSenderPort, nSessionID);
	}
};