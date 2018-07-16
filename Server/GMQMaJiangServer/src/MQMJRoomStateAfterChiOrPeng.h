#pragma once
#include "IGameRoomState.h"
#include "MQMJRoom.h"
#include "MQMJPlayerCard.h"
class MQMJRoomStateAfterChiOrPeng
	:public IGameRoomState
{
public:
	uint32_t getStateID()final { return eRoomState_AfterChiOrPeng; }

	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)override
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		setStateDuringTime(9999999);
		m_bPass = false;
		if (jsTranData["idx"].isNull() == false && jsTranData["idx"].isUInt())
		{
			m_nIdx = jsTranData["idx"].asUInt();
			if (((MQMJRoom*)getRoom())->onWaitPlayerActAfterCP(m_nIdx)) {
				//TODO...
			}
			else {
				Json::Value jsValue;
				jsValue["idx"] = m_nIdx;
				getRoom()->goToState(eRoomState_WaitPlayerChu, &jsValue);
			}
			return;
		}
		Assert(0, "invalid argument");
	}

	void onStateTimeUp()override
	{
		Json::Value jsValue;
		jsValue["idx"] = m_nIdx;
		getRoom()->goToState(eRoomState_WaitPlayerChu, &jsValue);
	}

	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override
	{
		if (MSG_REQ_ACT_LIST == nMsgType)
		{
			if (m_bPass) {
				LOGFMTE("already pass why req act list");
				return false;
			}

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

			if (((MQMJRoom*)getRoom())->onWaitPlayerActAfterCP(m_nIdx)) {
				//TODO...
			}
			else {
				m_bPass = true;
				setStateDuringTime(0);
			}
			return true;
		}

		if (MSG_PLAYER_ACT != nMsgType)
		{
			return false;
		}

		auto actType = prealMsg["actType"].asUInt();
		auto nCard = prealMsg["card"].asUInt();
		auto pPlayer = (IMJPlayer*)getRoom()->getPlayerBySessionID(nSessionID);
		uint8_t nRet = 0;
		do
		{
			if (m_bPass) {
				nRet = 5;
				break;
			}

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

			auto pMJCard = (MQMJPlayerCard*)pPlayer->getPlayerCard();
			switch (actType)
			{
			case eMJAct_Cyclone:
			{
				if (!pMJCard->canCycloneWithCard(nCard))
				{
					nRet = 3;
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
			getRoom()->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			return true;
		}

		if (eMJAct_Pass == actType)
		{
			setStateDuringTime(0);
			return true;
		}

		// do transfer 
		Json::Value jsTran;
		jsTran["idx"] = m_nIdx;
		jsTran["act"] = actType;
		jsTran["card"] = nCard;
		jsTran["invokeIdx"] = m_nIdx;
		getRoom()->goToState(eRoomState_DoPlayerAct, &jsTran);
		return true;
	}
	uint8_t getCurIdx()override { return m_nIdx; }

protected:
	uint8_t m_nIdx;
	bool m_bPass;
};