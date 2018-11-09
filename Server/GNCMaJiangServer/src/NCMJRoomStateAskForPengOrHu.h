#pragma once
#include "MJRoomStateAskForPengOrHu.h"
class NCMJRoomStateAskForPengOrHu
	:public MJRoomStateAskForPengOrHu
{
public:
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)override {
		m_bNeedSendMsg = true;
		m_bWaitingPeng = true;
		m_bWaitingEat = true;
		m_nCurrentIdx = -1;
		MJRoomStateAskForPengOrHu::enterState(pmjRoom, jsTranData);
	}

	void update(float fDeta) override {
		MJRoomStateAskForPengOrHu::update(fDeta);
		if (m_bNeedSendMsg == false) {
			return;
		}
		if (isWaitingPlayerAct()) {
			return;
		}

		auto pRoom = (IMJRoom*)getRoom();
		Json::Value jsTran;
		jsTran["idx"] = pRoom->getNextActPlayerIdx(m_nInvokeIdx);
		jsTran["act"] = eMJAct_Mo;
		pRoom->goToState(eRoomState_DoPlayerAct, &jsTran);
	}

	void responeReqActList(uint32_t nSessionID)override {
		auto pRoom = (IMJRoom*)getRoom();
		auto pPlayer = (IMJPlayer*)pRoom->getPlayerBySessionID(nSessionID);
		if (!pPlayer)
		{
			LOGFMTE("you are  not in room id = %u , session id = %u , can not send you act list", pRoom->getRoomID(), nSessionID);
			return;
		}

		if (pPlayer->getIdx() != m_nCurrentIdx) {
			LOGFMTE("you are not current ask for peng or hu, can not send you act list. session id = %u", nSessionID);
			return;
		}

		Json::Value jsMsg;
		jsMsg["invokerIdx"] = m_nInvokeIdx;
		jsMsg["cardNum"] = m_nCard;
		Json::Value jsActs;

		if (m_vWaitHuIdx.size()) {
			jsActs[jsActs.size()] = eMJAct_Hu;
		}

		if (m_bWaitingPeng == false && m_vWaitPengGangIdx.size()) {
			jsActs[jsActs.size()] = eMJAct_Peng;
			if (pRoom->canGang() && pPlayer->getPlayerCard()->canMingGangWithCard(m_nCard)) {
				jsActs[jsActs.size()] = eMJAct_MingGang;
			}
		}

		if (m_bWaitingEat == false && m_isNeedWaitEat) {
			jsActs[jsActs.size()] = eMJAct_Chi;
		}

		jsMsg["acts"] = jsActs;
		pRoom->sendMsgToPlayer(jsMsg, MSG_PLAYER_WAIT_ACT_ABOUT_OTHER_CARD, pPlayer->getSessionID());

		/*Json::Value jsFramePlayer, jsFrameArg;
		jsFramePlayer["idx"] = m_nCurrentIdx;
		jsFramePlayer["acts"] = jsActs;
		jsFrameArg[jsFrameArg.size()] = jsFramePlayer;
		auto ptrReplay = getRoom()->getGameReplay()->createFrame(eMJFrame_WaitPlayerActAboutCard, (uint32_t)time(nullptr));
		ptrReplay->setFrameArg(jsFrameArg);
		getRoom()->getGameReplay()->addFrame(ptrReplay);*/
	}

	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override {
		if (MSG_REQ_ACT_LIST == nMsgType)
		{
			responeReqActList(nSessionID);
			return true;
		}

		if (MSG_PLAYER_ACT == nMsgType) {
			auto pRoom = (IMJRoom*)getRoom();

			auto actType = prealMsg["actType"].asUInt();
			auto pPlayer = (IMJPlayer*)pRoom->getPlayerBySessionID(nSessionID);
			uint8_t nRet = 0;
			do
			{
				if (pPlayer == nullptr)
				{
					LOGFMTE("you are not in this room how to set ready ? session id = %u", nSessionID);
					nRet = 4;
					break;
				}

				if (m_nCurrentIdx != pPlayer->getIdx()) {
					LOGFMTE("you are not current ask for peng or hu ? session id = %u", nSessionID);
					nRet = 5;
					break;
				}

				if (eMJAct_Chi == actType)
				{
					if (m_isNeedWaitEat == false)
					{
						LOGFMTE("not wait eat act ");
						nRet = 1;
						break;
					}

					if (pPlayer->getIdx() != (m_nInvokeIdx + 1) % pRoom->getSeatCnt())
					{
						LOGFMTE("eat only just previous player's card");
						nRet = 1;
						break;
					}
				}
				else
				{
					auto iter = std::find(m_vWaitHuIdx.begin(), m_vWaitHuIdx.end(), pPlayer->getIdx());
					auto iterPeng = std::find(m_vWaitPengGangIdx.begin(), m_vWaitPengGangIdx.end(), pPlayer->getIdx());

					if (iter == m_vWaitHuIdx.end() && iterPeng == m_vWaitPengGangIdx.end() && m_isNeedWaitEat == false)
					{
						nRet = 1;
						break;
					}
				}


				if (eMJAct_Pass == actType)
				{
					if (onPlayerPass(pPlayer->getIdx()) == false) {
						nRet = 2;
					}
					break;
				}

				auto pMJCard = pPlayer->getPlayerCard();
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
					m_vDoHuIdx.push_back(pPlayer->getIdx());
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
					m_vDoPengGangIdx.push_back(pPlayer->getIdx());
					m_ePengGangAct = eMJAct_Peng;
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
					m_vDoPengGangIdx.push_back(pPlayer->getIdx());
					m_ePengGangAct = eMJAct_MingGang;
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
					if (!pMJCard->canEatCard(m_nCard, m_vEatWithInfo[0], m_vEatWithInfo[1]))
					{
						LOGFMTE("why you can not eat ? svr bug ");
						nRet = 2;
						m_vEatWithInfo.clear();
						break;
					}
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

			if (m_vDoHuIdx.size() || m_vDoPengGangIdx.size() || m_vEatWithInfo.size()) {
				doAct();
			}

			return true;
		}

		return false;
	}

protected:
	bool onPlayerPass(uint8_t nIdx) {
		auto pRoom = (IMJRoom*)getRoom();

		if (m_vWaitHuIdx.size()) {
			auto iter = std::find(m_vWaitHuIdx.begin(), m_vWaitHuIdx.end(), nIdx);
			if (iter == m_vWaitHuIdx.end()) {
				return false;
			}
			else {
				pRoom->onPlayerLouHu(*iter, m_nInvokeIdx);
				m_vWaitHuIdx.erase(iter);
			}
			m_bNeedSendMsg = true;
		}

		if (m_bWaitingPeng == false && m_vWaitPengGangIdx.size()) {
			auto iter = std::find(m_vWaitPengGangIdx.begin(), m_vWaitPengGangIdx.end(), nIdx);
			if (iter == m_vWaitPengGangIdx.end()) {
				return false;
			}
			else {
				pRoom->onPlayerLouPeng(*iter, m_nCard);
				m_vWaitPengGangIdx.clear();
			}
			m_bNeedSendMsg = true;
		}

		if (m_bWaitingEat == false && m_isNeedWaitEat) {
			m_isNeedWaitEat = false;
			m_bNeedSendMsg = true;
		}

		return true;
	}

	bool isWaitingPlayerAct() {
		auto pRoom = (IMJRoom*)getRoom();

		if (m_vWaitHuIdx.size()) {
			auto pIdx = m_vWaitHuIdx.front();
			auto pPlayer = (IMJPlayer*)pRoom->getPlayerByIdx(pIdx);
			if (pPlayer) {
				Json::Value jsMsg;
				jsMsg["invokerIdx"] = m_nInvokeIdx;
				jsMsg["cardNum"] = m_nCard;
				Json::Value jsActs;
				jsActs[jsActs.size()] = eMJAct_Hu;

				if (m_vWaitHuIdx.size() == 1) {
					if (m_vWaitPengGangIdx.size()) {
						if (m_vWaitPengGangIdx.front() == pIdx) {
							jsActs[jsActs.size()] = eMJAct_Peng;
							if (pRoom->canGang() && pPlayer->getPlayerCard()->canMingGangWithCard(m_nCard)) {
								jsActs[jsActs.size()] = eMJAct_MingGang;
							}
							m_bWaitingPeng = false;

							if (m_isNeedWaitEat && pIdx == (m_nInvokeIdx + 1) % pRoom->getSeatCnt()) {
								jsActs[jsActs.size()] = eMJAct_Chi;
								m_bWaitingEat = false;
							}
						}
					}
					else {
						if (m_isNeedWaitEat && pIdx == (m_nInvokeIdx + 1) % pRoom->getSeatCnt()) {
							jsActs[jsActs.size()] = eMJAct_Chi;
							m_bWaitingEat = false;
						}
					}
				}

				if (jsActs.size() > 0)
				{
					jsActs[jsActs.size()] = eMJAct_Pass;
				}

				jsMsg["acts"] = jsActs;
				pRoom->sendMsgToPlayer(jsMsg, MSG_PLAYER_WAIT_ACT_ABOUT_OTHER_CARD, pPlayer->getSessionID());

				Json::Value jsFramePlayer, jsFrameArg;
				jsFramePlayer["idx"] = pIdx;
				jsFramePlayer["acts"] = jsActs;
				jsFrameArg[jsFrameArg.size()] = jsFramePlayer;
				pRoom->addReplayFrame(eMJFrame_WaitPlayerActAboutCard, jsFrameArg);

				m_nCurrentIdx = pIdx;
				m_bNeedSendMsg = false;
			}
			else {
				m_vWaitHuIdx.erase(m_vWaitHuIdx.begin());
			}
			return true;
		}

		if (m_vWaitPengGangIdx.size() == 1) {
			auto pIdx = m_vWaitPengGangIdx.front();
			auto pPlayer = (IMJPlayer*)pRoom->getPlayerByIdx(pIdx);
			if (pPlayer) {
				Json::Value jsMsg;
				jsMsg["invokerIdx"] = m_nInvokeIdx;
				jsMsg["cardNum"] = m_nCard;
				Json::Value jsActs;
				jsActs[jsActs.size()] = eMJAct_Peng;
				if (pRoom->canGang() && pPlayer->getPlayerCard()->canMingGangWithCard(m_nCard)) {
					jsActs[jsActs.size()] = eMJAct_MingGang;
				}
				if (m_isNeedWaitEat && pIdx == (m_nInvokeIdx + 1) % pRoom->getSeatCnt()) {
					jsActs[jsActs.size()] = eMJAct_Chi;
					m_bWaitingEat = false;
				}
				if (jsActs.size() > 0)
				{
					jsActs[jsActs.size()] = eMJAct_Pass;
				}
				jsMsg["acts"] = jsActs;
				pRoom->sendMsgToPlayer(jsMsg, MSG_PLAYER_WAIT_ACT_ABOUT_OTHER_CARD, pPlayer->getSessionID());

				Json::Value jsFramePlayer, jsFrameArg;
				jsFramePlayer["idx"] = pIdx;
				jsFramePlayer["acts"] = jsActs;
				jsFrameArg[jsFrameArg.size()] = jsFramePlayer;
				pRoom->addReplayFrame(eMJFrame_WaitPlayerActAboutCard, jsFrameArg);

				m_nCurrentIdx = pIdx;
				m_bWaitingPeng = false;
				m_bNeedSendMsg = false;
			}
			else {
				m_vWaitPengGangIdx.clear();
			}
			return true;
		}

		if (m_isNeedWaitEat) {
			auto pIdx = (m_nInvokeIdx + 1) % pRoom->getSeatCnt();
			auto pPlayer = (IMJPlayer*)pRoom->getPlayerByIdx(pIdx);
			if (pPlayer) {
				Json::Value jsMsg;
				jsMsg["invokerIdx"] = m_nInvokeIdx;
				jsMsg["cardNum"] = m_nCard;
				Json::Value jsActs;
				jsActs[jsActs.size()] = eMJAct_Chi;
				jsMsg["acts"] = jsActs;
				pRoom->sendMsgToPlayer(jsMsg, MSG_PLAYER_WAIT_ACT_ABOUT_OTHER_CARD, pPlayer->getSessionID());

				Json::Value jsFramePlayer, jsFrameArg;
				jsFramePlayer["idx"] = pIdx;
				jsFramePlayer["acts"] = jsActs;
				jsFrameArg[jsFrameArg.size()] = jsFramePlayer;
				pRoom->addReplayFrame(eMJFrame_WaitPlayerActAboutCard, jsFrameArg);

				m_nCurrentIdx = pIdx;
				m_bWaitingEat = false;
				m_bNeedSendMsg = false;
			}
			else {
				m_isNeedWaitEat = false;
			}
			return true;
		}

		return false;
	}

protected:
	bool m_bNeedSendMsg;
	bool m_bWaitingPeng;
	bool m_bWaitingEat;
	uint8_t m_nCurrentIdx;
};