#pragma once
#include "MJRoomStateAskForPengOrHu.h"
class FXMJRoomStateAskForPengOrHu
	:public MJRoomStateAskForPengOrHu
{
public:
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
		auto pPlayer = getRoom()->getPlayerBySessionID(nSessionID);
		uint8_t nRet = 0;
		do
		{
			if (pPlayer == nullptr)
			{
				LOGFMTE("you are not in this room how to set ready ? session id = %u", nSessionID);
				nRet = 4;
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

				if (pPlayer->getIdx() != (m_nInvokeIdx + 1) % getRoom()->getSeatCnt())
				{
					LOGFMTE("eat only just previous player's card");
					nRet = 1;
					break;
				}
			}
			else if (eMJAct_Pass != actType)
			{
				auto iter = std::find(m_vWaitHuIdx.begin(), m_vWaitHuIdx.end(), pPlayer->getIdx());
				auto iterPeng = std::find(m_vWaitPengGangIdx.begin(), m_vWaitPengGangIdx.end(), pPlayer->getIdx());

				if (iter == m_vWaitHuIdx.end() && iterPeng == m_vWaitPengGangIdx.end())
				{
					nRet = 1;
					break;
				}
			}


			if (eMJAct_Pass == actType)
			{
				if (m_isNeedWaitEat && pPlayer->getIdx() == (m_nInvokeIdx + 1) % getRoom()->getSeatCnt())
				{
					LOGFMTD("give up eat act");
					m_isNeedWaitEat = false;
				}
				break;
			}

			auto pMJCard = ((IMJPlayer*)pPlayer)->getPlayerCard();
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
				m_ePengGangAct = eMJAct_MingGang_Pre;
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
				if (!pMJCard->canEatCard(m_nCard))
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
			getRoom()->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			return true;
		}

		auto iter = std::find(m_vWaitHuIdx.begin(), m_vWaitHuIdx.end(), pPlayer->getIdx());
		if (iter != m_vWaitHuIdx.end())
		{
			m_vWaitHuIdx.erase(iter);

			// inform lou hu 
			if (eMJAct_Pass == actType)
			{
				((IMJRoom*)getRoom())->onPlayerLouHu(pPlayer->getIdx(), m_nInvokeIdx);
			}
		}

		auto iterPeng = std::find(m_vWaitPengGangIdx.begin(), m_vWaitPengGangIdx.end(), pPlayer->getIdx());
		if (iterPeng != m_vWaitPengGangIdx.end())
		{
			m_vWaitPengGangIdx.erase(iterPeng);

			// inform lou peng 
			if (eMJAct_Pass == actType)
			{
				((IMJRoom*)getRoom())->onPlayerLouPeng(pPlayer->getIdx(), m_nCard);
			}
		}

		if (m_vWaitHuIdx.empty() == false)  // wait hu ;
		{
			return true;
		}

		if (m_vDoHuIdx.empty() && m_vWaitPengGangIdx.empty() == false)  // wait peng gang ;
		{
			return true;
		}

		if (m_vDoHuIdx.empty() && m_vDoPengGangIdx.empty() && m_isNeedWaitEat && m_vEatWithInfo.empty())  // wait eat
		{
			return true;
		}

		if (m_vDoHuIdx.empty() && m_vDoPengGangIdx.empty() && m_isNeedWaitEat == false)
		{
			onStateTimeUp();  // the same as wait time out , all candinate give up ;
			return true;
		}

		doAct();
		return true;
	}
};