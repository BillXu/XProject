#pragma once
#include "MJRoomStateAskForRobotGang.h"
#include "SZMJRoom.h"
class SZMJRoomStateAskForRobotGang
	:public MJRoomStateAskForRobotGang
{
public:
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)override
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		setStateDuringTime(100000000);
		m_nInvokeIdx = jsTranData["invokeIdx"].asUInt();
		m_nCard = jsTranData["card"].asUInt();
		m_nActIdx = jsTranData["idx"].asUInt();
		m_nActType = jsTranData["act"].asUInt();
		m_vWaitIdx.clear();
		auto pRoom = (SZMJRoom*)getRoom();
		pRoom->onAskForRobotGang(m_nInvokeIdx, m_nCard, m_vWaitIdx);
		assert(m_vWaitIdx.empty() == false && "invalid argument");
		m_vDoHuIdx.clear();

		Json::Value jsMsg, jsWait;
		for (auto& ref : m_vWaitIdx) {
			jsWait[jsWait.size()] = ref;
		}
		jsMsg["waitIdx"] = jsWait;
		pmjRoom->sendRoomMsg(jsMsg, MSG_ROOM_PLAYER_WAIT_IDX);
	}

	void responeReqActList(uint32_t nSessionID)override {
		MJRoomStateAskForRobotGang::responeReqActList(nSessionID);
		Json::Value jsMsg, jsWait;
		for (auto& ref : m_vWaitIdx) {
			jsWait[jsWait.size()] = ref;
		}
		jsMsg["waitIdx"] = jsWait;
		getRoom()->sendMsgToPlayer(jsMsg, MSG_ROOM_PLAYER_WAIT_IDX, nSessionID);
	}

	void update(float fDeta)override
	{
		if (getWaitTime() > 15.0f) {
			for (auto& ref : m_vWaitIdx) {
				auto pPlayer = (SZMJPlayer*)getRoom()->getPlayerByIdx(ref);
				pPlayer->addExtraTime(fDeta);
			}
		}

		MJRoomStateAskForRobotGang::update(fDeta);
	}

	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override
	{
		if (MSG_PLAYER_ACT != nMsgType && MSG_REQ_ACT_LIST != nMsgType)
		{
			return false;
		}

		if (MSG_REQ_ACT_LIST == nMsgType)
		{
			responeReqActList(nSessionID);
			return true;
		}

		auto actType = prealMsg["actType"].asUInt();
		//auto nCard = prealMsg["card"].asUInt();
		auto pPlayer = (SZMJPlayer*)getRoom()->getPlayerBySessionID(nSessionID);
		uint8_t nRet = 0;
		do
		{
			if (pPlayer == nullptr)
			{
				LOGFMTE("you are not in this room how to set ready ? session id = %u", nSessionID);
				nRet = 4;
				break;
			}

			auto iter = std::find(m_vWaitIdx.begin(), m_vWaitIdx.end(), pPlayer->getIdx());

			if (iter == m_vWaitIdx.end())
			{
				nRet = 1;
				break;
			}

			if (eMJAct_Pass == actType)
			{
				break;
			}

			if (eMJAct_Hu != actType)
			{
				nRet = 2;
				break;
			}

			auto pMJCard = (SZMJPlayerCard*)((IMJPlayer*)pPlayer)->getPlayerCard();
			if (!pMJCard->canHuWitCard(m_nCard, true))
			{
				nRet = 2;
				LOGFMTE("why you can not hu ? svr bug ");
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

		auto iter = std::find(m_vWaitIdx.begin(), m_vWaitIdx.end(), pPlayer->getIdx());
		m_vWaitIdx.erase(iter);
		if (eMJAct_Pass != actType)
		{
			m_vDoHuIdx.push_back(pPlayer->getIdx());
		}

		if (m_vWaitIdx.empty() == false)
		{
			Json::Value jsMsg, jsWait;
			for (auto& ref : m_vWaitIdx) {
				jsWait[jsWait.size()] = ref;
			}
			jsMsg["waitIdx"] = jsWait;
			getRoom()->sendRoomMsg(jsMsg, MSG_ROOM_PLAYER_WAIT_IDX);
			return true;
		}

		if (m_vDoHuIdx.empty())
		{
			LOGFMTD("every one give up robot gang");

			Json::Value jsTran;
			jsTran["idx"] = m_nInvokeIdx;
			jsTran["card"] = m_nCard;
			jsTran["act"] = eMJAct_BuGang;
			getRoom()->goToState(eRoomState_DoPlayerAct, &jsTran);
		}
		else
		{
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
			getRoom()->goToState(eRoomState_DoPlayerAct, &jsTran);
		}

		return true;
	}

protected:
	uint8_t m_nActType;
	uint8_t m_nActIdx;
};