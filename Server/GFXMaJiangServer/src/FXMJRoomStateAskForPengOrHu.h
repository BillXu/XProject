#pragma once
#include "MJRoomStateAskForPengOrHu.h"
class FXMJRoomStateAskForPengOrHu
	:public MJRoomStateAskForPengOrHu
{
public:
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)override
	{
		m_nTing = 0;
		MJRoomStateAskForPengOrHu::enterState(pmjRoom, jsTranData);
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

			auto pMJCard = (FXMJPlayerCard*)((IMJPlayer*)pPlayer)->getPlayerCard();
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
				m_nTing = prealMsg["ting"].isUInt() ? prealMsg["ting"].asUInt() : 0;
				m_vDoPengGangIdx.push_back(pPlayer->getIdx());
				m_ePengGangAct = eMJAct_MingGang_Pre;
				pMJCard->addPreGang(m_nCard);
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
				((IMJRoom*)getRoom())->onPlayerLouHu(pPlayer->getIdx(), m_nCard);
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

	bool doAct() override
	{
		if (m_vDoHuIdx.empty() == false)
		{
			LOGFMTD("some body do hu ");
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
			return true;
		}
		else if (!m_vDoPengGangIdx.empty())
		{
			// gang or peng ;
			Json::Value jsTran;
			jsTran["idx"] = m_vDoPengGangIdx.front();
			jsTran["act"] = m_ePengGangAct;
			jsTran["card"] = m_nCard;
			jsTran["invokeIdx"] = m_nInvokeIdx;
			jsTran["ting"] = m_nTing;
			getRoom()->goToState(eRoomState_DoPlayerAct, &jsTran);
			return true;
		}

		// must do eat
		if (m_vEatWithInfo.empty() == false)
		{
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
		return false;
	}

protected:
	uint8_t m_nTing;
};