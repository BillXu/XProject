#pragma once
#include "MJRoomStateWaitPlayerChu.h"
class ARQMJRoomStateWaitPlayerChu
	:public MJRoomStateWaitPlayerChu
{
public:
	void update(float fDeta)override
	{
		if (getWaitTime() > 15.0f) {
			auto pPlayer = (ARQMJPlayer*)getRoom()->getPlayerByIdx(m_nIdx);
			pPlayer->addExtraTime(fDeta);
		}
		MJRoomStateWaitPlayerChu::update(fDeta);
	}

	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override
	{
		if (MSG_PLAYER_ACT != nMsgType)
		{
			return false;
		}

		auto actType = prealMsg["actType"].asUInt();
		auto nCard = prealMsg["card"].asUInt();
		auto pPlayer = (ARQMJPlayer*)getRoom()->getPlayerBySessionID(nSessionID);
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

			if (eMJAct_Chu != actType)
			{
				nRet = 2;
				break;
			}

			auto pMJCard = ((IMJPlayer*)pPlayer)->getPlayerCard();
			if (!pMJCard->isHaveCard(nCard))
			{
				nRet = 3;
				break;
			}

			/*auto nType = card_Type(nCard);
			bool isHua = (eCT_Jian == nType || eCT_Hua == nType);
			if (isHua)
			{
			nRet = 4;
			LOGFMTE("can not chu hua = %u", nCard);
			break;
			}*/

		} while (0);

		if (nRet)
		{
			Json::Value jsRet;
			jsRet["ret"] = nRet;
			getRoom()->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
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
};