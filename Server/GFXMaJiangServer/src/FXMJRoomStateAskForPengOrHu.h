#pragma once
#include "MJRoomStateAskForPengOrHu.h"
class FXMJRoomStateAskForPengOrHu
	:public IGameRoomState
{
public:
	uint32_t getStateID()final { return eRoomState_AskForHuAndPeng; }
	uint8_t getCurIdx()override { return m_nCurActIdx; }

	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)override
	{
		m_nTing = 0;
		m_nWaitPeng = -1;
		m_nPengOrGang = eMJAct_None;
		m_vAlreadyActIdx.clear();
		m_nCurAct = eMJAct_Hu;
		m_nCurActIdx = -1;
		m_bWaitEat = false;
		m_vEatWithInfo.clear();
		m_bPass = false;
		m_bCheckPeng = false;
		m_bCheckEat = false;

		IGameRoomState::enterState(pmjRoom, jsTranData);
		setStateDuringTime(100000000);

		m_nInvokeIdx = jsTranData["invokeIdx"].asUInt();
		m_nCard = jsTranData["card"].asUInt();

		auto pRoom = (FXMJRoom*)getRoom();
		pRoom->onAskForHuThisCard(m_nInvokeIdx, m_nCard, m_nCurActIdx, m_vAlreadyActIdx);
		if ((uint8_t)-1 == m_nCurActIdx) {
			m_nCurAct = eMJAct_Peng;
			pRoom->onAskForPengGangThisCard(m_nInvokeIdx, m_nCard, m_nCurActIdx, m_vAlreadyActIdx);
			m_bCheckPeng = true;
			if ((uint8_t)-1 == m_nCurActIdx) {
				m_nCurAct = eMJAct_Chi;
				pRoom->onAskForEatThisCard(m_nInvokeIdx, m_nCard, m_nCurActIdx, m_vAlreadyActIdx);
				m_bCheckEat = true;
				if ((uint8_t)-1 == m_nCurActIdx) {
					assert(false && "wait nobody act with other card");
				}
			}
		}
	}

	void responeReqActList(uint32_t nSessionID)
	{
		auto pPlayer = (FXMJPlayer*)getRoom()->getPlayerBySessionID(nSessionID);

		if (!pPlayer)
		{
			LOGFMTE("you are  not in room id = %u , session id = %u , can not send you act list", getRoom()->getRoomID(), nSessionID);
			return;
		}

		if (pPlayer->getIdx() != m_nCurActIdx) {
			//LOGFMTE("you are  not in room id = %u , session id = %u , can not send you act list", getRoom()->getRoomID(), nSessionID);
			return;
		}

		Json::Value jsMsg;
		jsMsg["invokerIdx"] = m_nInvokeIdx;
		jsMsg["cardNum"] = m_nCard;

		Json::Value jsActs;
		auto pRoom = (FXMJRoom*)getRoom();

		if (m_nCurAct == eMJAct_Hu) {
			jsActs[jsActs.size()] = eMJAct_Hu;
		}

		if (m_nCurAct >= eMJAct_Peng) {
			if (pPlayer->getPlayerCard()->canPengWithCard(m_nCard)) {
				jsActs[jsActs.size()] = eMJAct_Peng;
			}
			
			if (pRoom->isCanGoOnMoPai() && pPlayer->getPlayerCard()->canMingGangWithCard(m_nCard))
			{
				jsActs[jsActs.size()] = eMJAct_MingGang;
			}
		}

		if (m_nCurAct >= eMJAct_Chi) {
			if (pPlayer->getIdx() == (m_nInvokeIdx + 1) % pRoom->getSeatCnt() && pPlayer->getPlayerCard()->canEatCard(m_nCard)) {
				jsActs[jsActs.size()] = eMJAct_Chi;
			}
		}

		if (jsActs.empty())
		{
			assert(false && "respone no act with other card");
			return;
		}

		if (jsActs.size() > 0)
		{
			jsActs[jsActs.size()] = eMJAct_Pass;
		}

		jsMsg["acts"] = jsActs;
		getRoom()->sendMsgToPlayer(jsMsg, MSG_PLAYER_WAIT_ACT_ABOUT_OTHER_CARD, nSessionID);
		LOGFMTD("respon act list inform uid = %u act about other card room id = %u card = %u", pPlayer->getUserUID(), getRoom()->getRoomID(), m_nCard);
	}

	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override
	{
		if (MSG_PLAYER_ACT != nMsgType && MSG_REQ_ACT_LIST != nMsgType)
		{
			return false;
		}

		if (m_bPass) {
			LOGFMTE("you are already chose act in room id = %u , session id = %u , can not do again", getRoom()->getRoomID(), nSessionID);
			return true;
		}

		if (MSG_REQ_ACT_LIST == nMsgType)
		{
			responeReqActList(nSessionID);
			return true;
		}

		auto actType = prealMsg["actType"].asUInt();
		//auto nCard = prealMsg["card"].asUInt();
		auto pRoom = (FXMJRoom*)getRoom();
		auto pPlayer = (FXMJPlayer*)pRoom->getPlayerBySessionID(nSessionID);
		auto pMJCard = (FXMJPlayerCard*)pPlayer->getPlayerCard();
		uint8_t nRet = 0;
		do
		{
			if (pPlayer == nullptr)
			{
				LOGFMTE("you are not in this room how to set ready ? session id = %u", nSessionID);
				nRet = 4;
				break;
			}

			if (pPlayer->getIdx() != m_nCurActIdx) {
				LOGFMTE("you are not current act idx ? id = %u", pPlayer->getUserUID());
				nRet = 4;
				break;
			}

			switch (actType)
			{
			case eMJAct_Hu:
			{
				if (!pMJCard->canHuWitCard(m_nCard))
				{
					nRet = 2;
					LOGFMTE("why you can not hu ? svr bug ");
					break;
				}
			}
			break;
			case eMJAct_Peng:
			{
				if (!pMJCard->canPengWithCard(m_nCard))
				{
					nRet = 2;
					LOGFMTE("why you can not peng ? svr bug ");
					break;
				}
				m_nPengOrGang = eMJAct_Peng;
				m_nWaitPeng = pPlayer->getIdx();
				if (m_nCurAct == eMJAct_Hu) {
					m_bPass = true;
					//m_vAlreadyActIdx.push_back(pPlayer->getIdx());
				}
			}
			break;
			case eMJAct_MingGang:
			{
				if (!pMJCard->canMingGangWithCard(m_nCard))
				{
					nRet = 2;
					LOGFMTE("why you can not ming gang ? svr bug ");
					break;
				}
				m_nTing = prealMsg["ting"].isUInt() ? prealMsg["ting"].asUInt() : 0;
				m_nPengOrGang = eMJAct_MingGang_Pre;
				m_nWaitPeng = pPlayer->getIdx();
				pMJCard->addPreGang(m_nCard);
				if (m_nCurAct == eMJAct_Hu) {
					m_bPass = true;
					//m_vAlreadyActIdx.push_back(pPlayer->getIdx());
				}
			}
			break;
			case eMJAct_Chi:
			{
				if (prealMsg["eatWith"].isNull() || prealMsg["eatWith"].isArray() == false || prealMsg["eatWith"].size() != 2)
				{
					LOGFMTE("eat arg error");
					nRet = 3;
					break;
				}
				Json::Value jsE;
				jsE = prealMsg["eatWith"];
				m_vEatWithInfo.clear();
				m_vEatWithInfo.push_back(jsE[0u].asUInt());
				m_vEatWithInfo.push_back(jsE[1u].asUInt());
				if (pPlayer->getIdx() != (m_nInvokeIdx + 1) % pRoom->getSeatCnt() || !pMJCard->canEatCard(m_nCard, m_vEatWithInfo[0], m_vEatWithInfo[1]))
				{
					LOGFMTE("why you can not eat ? svr bug ");
					nRet = 2;
					m_vEatWithInfo.clear();
					break;
				}
				m_bWaitEat = true;
				if (m_nCurAct > eMJAct_Chi) {
					m_bPass = true;
					//m_vAlreadyActIdx.push_back(pPlayer->getIdx());
				}
			}
			break;
			case eMJAct_Pass:
			{
				m_bPass = true;
			}
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

		m_vAlreadyActIdx.push_back(pPlayer->getIdx());

		if (m_nCurAct == eMJAct_Hu && actType != eMJAct_Hu) {
			pRoom->onPlayerLouHu(pPlayer->getIdx(), m_nCard);
		}

		if (m_nCurAct >= eMJAct_Peng && pMJCard->canPengWithCard(m_nCard) && actType != eMJAct_Peng && actType != eMJAct_MingGang) {
			pRoom->onPlayerLouPeng(pPlayer->getIdx(), m_nCard);
		}

		setStateDuringTime(0);
		return true;
	}

	bool doAct()
	{
		if (m_bPass) {
			return false;
		}

		switch (m_nCurAct)
		{
		case eMJAct_Hu:
		{
			LOGFMTD("some body do hu ");
			// do transfer 
			Json::Value jsTran;
			jsTran["idx"] = m_nCurActIdx;
			jsTran["act"] = eMJAct_Hu;
			jsTran["card"] = m_nCard;
			jsTran["invokeIdx"] = m_nInvokeIdx;

			Json::Value jsHuIdx;
			jsHuIdx[jsHuIdx.size()] = m_nCurActIdx;
			jsTran["huIdxs"] = jsHuIdx;
			getRoom()->goToState(eRoomState_DoPlayerAct, &jsTran);
			return true;
		}
		break;
		case eMJAct_Peng:
		{
			if (uint8_t(-1) != m_nWaitPeng && eMJAct_None != m_nPengOrGang) {
				// gang or peng ;
				Json::Value jsTran;
				jsTran["idx"] = m_nWaitPeng;
				jsTran["act"] = m_nPengOrGang;
				jsTran["card"] = m_nCard;
				jsTran["invokeIdx"] = m_nInvokeIdx;
				jsTran["ting"] = m_nTing;
				getRoom()->goToState(eRoomState_DoPlayerAct, &jsTran);
				return true;
			}
		}
		break;
		case eMJAct_Chi:
		{
			if (m_bWaitEat && m_vEatWithInfo.empty() == false) {
				Json::Value jsTran;
				jsTran["idx"] = (m_nInvokeIdx + 1) % getRoom()->getSeatCnt();
				jsTran["act"] = eMJAct_Chi;
				jsTran["card"] = m_nCard;
				jsTran["invokeIdx"] = m_nInvokeIdx;
				jsTran["eatWithA"] = m_vEatWithInfo[0];
				jsTran["eatWithB"] = m_vEatWithInfo[1];
				getRoom()->goToState(eRoomState_DoPlayerAct, &jsTran);
				return true;
			}
		}
		break;
		}
		return false;
	}

	void reEnter()
	{
		m_nCurActIdx = -1;
		m_bPass = false;
	}

	void onStateTimeUp()override
	{
		if (doAct())
		{
			return;
		}

		reEnter();
		auto pRoom = (FXMJRoom*)getRoom();
		switch (m_nCurAct)
		{
		case eMJAct_Hu:
		{
			pRoom->onAskForHuThisCard(m_nInvokeIdx, m_nCard, m_nCurActIdx, m_vAlreadyActIdx);
			if ((uint8_t)-1 == m_nCurActIdx) {
				m_nCurAct = eMJAct_Peng;
				setStateDuringTime(0);
				return;
			}
			else {
				setStateDuringTime(100000000);
				return;
			}
		}
		break;
		case eMJAct_Peng:
		{
			if (m_bCheckPeng) {
				m_nCurAct = eMJAct_Chi;
				setStateDuringTime(0);
				return;
			}
			else {
				pRoom->onAskForPengGangThisCard(m_nInvokeIdx, m_nCard, m_nCurActIdx, m_vAlreadyActIdx);
				m_bCheckPeng = true;
				if ((uint8_t)-1 == m_nCurActIdx) {
					m_nCurAct = eMJAct_Chi;
					setStateDuringTime(0);
					return;
				}
				else {
					setStateDuringTime(100000000);
					return;
				}
			}
		}
		break;
		case eMJAct_Chi:
		{
			if (m_bCheckEat) {
				break;
			}
			pRoom->onAskForEatThisCard(m_nInvokeIdx, m_nCard, m_nCurActIdx, m_vAlreadyActIdx);
			m_bCheckEat = true;
			if ((uint8_t)-1 == m_nCurActIdx) {
				break;
			}
			else {
				setStateDuringTime(100000000);
				return;
			}
		}
		break;
		}

		if (pRoom->isGameOver())
		{
			pRoom->goToState(eRoomState_GameEnd);
			return;
		}

		Json::Value jsTran;
		jsTran["idx"] = pRoom->getNextActPlayerIdx(m_nInvokeIdx);
		jsTran["act"] = eMJAct_Mo;
		pRoom->goToState(eRoomState_DoPlayerAct, &jsTran);
	}
protected:
	uint8_t m_nTing;
	uint8_t m_nInvokeIdx;
	uint8_t m_nCard;
	bool m_bCheckPeng;
	bool m_bCheckEat;

	uint8_t m_nCurActIdx;
	bool m_bPass;
	eMJActType m_nCurAct;
	std::vector<uint8_t> m_vAlreadyActIdx;

	eMJActType m_nPengOrGang;
	uint8_t m_nWaitPeng;
	bool m_bWaitEat;
	std::vector<uint8_t> m_vEatWithInfo;
};