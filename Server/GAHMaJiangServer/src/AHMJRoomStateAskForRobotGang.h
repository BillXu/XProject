#pragma once
#include "MJRoomStateAskForRobotGang.h"
#include "MJReplayFrameType.h"
#include <ctime>
class AHMJRoomStateAskForRobotGang
	:public MJRoomStateAskForRobotGang
{
public:
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)override
	{
		m_bNeedSendMsg = true;
		m_nCurrentIdx = -1;
		MJRoomStateAskForRobotGang::enterState(pmjRoom, jsTranData);
	}

	void update(float fDeta) override
	{
		MJRoomStateAskForRobotGang::update(fDeta);
		if (m_bNeedSendMsg == false) {
			return;
		}
		if (isWaitingPlayerAct()) {
			return;
		}

		setStateDuringTime(0);
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
			auto actType = prealMsg["actType"].asUInt();
			auto pRoom = (IMJRoom*)getRoom();
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

				auto iterPeng = std::find(m_vWaitIdx.begin(), m_vWaitIdx.end(), m_nCurrentIdx);

				if (iterPeng == m_vWaitIdx.end())
				{
					nRet = 1;
					break;
				}


				if (eMJAct_Pass == actType)
				{
					if (onPlayerPass(pPlayer->getIdx()) == false) {
						nRet = 2;
					}
					break;
				}

				if (eMJAct_Hu != actType)
				{
					nRet = 2;
					break;
				}

				auto pMJCard = pPlayer->getPlayerCard();
				if (!pMJCard->canHuWitCard(m_nCard))
				{
					nRet = 2;
					LOGFMTE("why you can not hu ? svr bug ");
					break;
				}

				m_vWaitIdx.erase(iterPeng);
				m_vDoHuIdx.push_back(m_nCurrentIdx);

			} while (0);

			if (nRet)
			{
				Json::Value jsRet;
				jsRet["ret"] = nRet;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				return true;
			}

			if (m_vDoHuIdx.size()) {
				LOGFMTD("some body do robot gang");
				// do transfer 
				Json::Value jsTran;
				jsTran["idx"] = m_vDoHuIdx.front();
				jsTran["act"] = eMJAct_Hu;
				jsTran["card"] = m_nCard;
				jsTran["invokeIdx"] = m_nInvokeIdx;

				Json::Value jsHuIdx;
				for (auto& ref : m_vDoHuIdx)
				{
					jsHuIdx[jsHuIdx.size()] = ref;
				}
				jsTran["huIdxs"] = jsHuIdx;
				pRoom->goToState(eRoomState_DoPlayerAct, &jsTran);
			}

			return true;
		}

		return false;
	}

protected:
	bool onPlayerPass(uint8_t nIdx) {
		if (m_vWaitIdx.size()) {
			auto iter = std::find(m_vWaitIdx.begin(), m_vWaitIdx.end(), nIdx);
			if (iter == m_vWaitIdx.end()) {
				return false;
			}
			else {
				auto pRoom = (IMJRoom*)getRoom();
				pRoom->onPlayerLouHu(nIdx, m_nInvokeIdx);
				m_vWaitIdx.erase(iter);
			}
			m_bNeedSendMsg = true;
		}

		return true;
	}

	bool isWaitingPlayerAct() {
		if (m_vWaitIdx.size()) {
			auto pIdx = m_vWaitIdx.front();
			auto pRoom = (IMJRoom*)getRoom();
			auto pPlayer = (IMJPlayer*)pRoom->getPlayerByIdx(pIdx);
			if (pPlayer) {
				Json::Value jsMsg;
				jsMsg["invokerIdx"] = m_nInvokeIdx;
				jsMsg["cardNum"] = m_nCard;
				Json::Value jsActs;
				jsActs[jsActs.size()] = eMJAct_Hu;

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
				m_vWaitIdx.erase(m_vWaitIdx.begin());
			}
			return true;
		}

		return false;
	}

protected:
	bool m_bNeedSendMsg;
	uint8_t m_nCurrentIdx;
};