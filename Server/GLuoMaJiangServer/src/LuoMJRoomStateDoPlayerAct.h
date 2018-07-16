#pragma once
#include "MJRoomStateDoPlayerAct.h"
#include "LuoMJRoom.h"
class LuoMJRoomStateDoPlayerAct
	:public MJRoomStateDoPlayerAct
{
public:
	void onStateTimeUp() override
	{
		switch (m_eActType)
		{
		case eMJAct_Chi:
		case eMJAct_Peng:
		{
			Json::Value jsValue;
			jsValue["idx"] = m_nActIdx;
			getRoom()->goToState(eRoomState_AfterChiOrPeng, &jsValue);
		}
		break;
		case eMJAct_Mo:
		case eMJAct_BuGang:
		case eMJAct_BuGang_Declare:
		case eMJAct_AnGang:
		case eMJAct_MingGang:
		case eMJAct_Cyclone:
		{
			Json::Value jsValue;
			jsValue["idx"] = m_nActIdx;
			getRoom()->goToState(eRoomState_WaitPlayerAct, &jsValue);
		}
		break;
		case eMJAct_MingGang_Pre:
		{
			Json::Value jsTran;
			jsTran["idx"] = m_nInvokeIdx;
			jsTran["act"] = eMJAct_MingGang_Pre;
			jsTran["card"] = m_nCard;
			jsTran["invokeIdx"] = m_nActIdx;
			getRoom()->goToState(eRoomState_AskForRobotGang, &jsTran);
		}
		break;
		case eMJAct_Hu:
		{
			if (((IMJRoom*)getRoom())->isGameOver())
			{
				getRoom()->goToState(eRoomState_GameEnd);
				return;
			}

			uint8_t nIdx = 0;
			for (auto& ref : m_vHuIdxs)
			{
				if (ref > nIdx)
				{
					nIdx = ref;
				}
			}

			do
			{
				nIdx = ((IMJRoom*)getRoom())->getNextActPlayerIdx(nIdx);
				LOGFMTD("next act player should not in hu list, try next");
			} while (std::find(m_vHuIdxs.begin(), m_vHuIdxs.end(), nIdx) != m_vHuIdxs.end());


			// next player mo pai 
			doNextPlayerMoPai(nIdx);
		}
		break;
		case eMJAct_Chu:
			if (((IMJRoom*)getRoom())->isAnyPlayerPengOrHuThisCard(m_nActIdx, m_nCard))
			{
				Json::Value jsValue;
				jsValue["invokeIdx"] = m_nActIdx;
				jsValue["card"] = m_nCard;
				getRoom()->goToState(eRoomState_AskForHuAndPeng, &jsValue);
				return;
			}

			if (((IMJRoom*)getRoom())->isGameOver())
			{
				getRoom()->goToState(eRoomState_GameEnd);
				return;
			}

			{
				auto nIdx = ((IMJRoom*)getRoom())->getNextActPlayerIdx(m_nActIdx);

				// next player mo pai 
				doNextPlayerMoPai(nIdx);
			}
			break;
		default:
			break;
		}
	}

protected:
	void doAct() override
	{
		auto pRoom = ((LuoMJRoom*)getRoom());
		switch (m_eActType)
		{
		case eMJAct_Mo:
			pRoom->onPlayerMo(m_nActIdx);
			break;
		case eMJAct_Peng:
			pRoom->onPlayerPeng(m_nActIdx, m_nCard, m_nInvokeIdx);
			break;
		case eMJAct_MingGang:
			pRoom->onPlayerMingGang(m_nActIdx, m_nCard, m_nInvokeIdx);
			break;
		case eMJAct_MingGang_Pre:
		{
			if (pRoom->isPlayerRootDirectGang(m_nInvokeIdx, m_nCard) == false) {
				pRoom->onPlayerMingGang(m_nActIdx, m_nCard, m_nInvokeIdx);
				m_eActType = eMJAct_MingGang;
			}
			break;
		}
		case eMJAct_AnGang:
			pRoom->onPlayerAnGang(m_nActIdx, m_nCard);
			break;
		case eMJAct_Cyclone:
			pRoom->onPlayerCyclone(m_nActIdx, m_nCard);
			break;
		case eMJAct_BuGang:
			pRoom->onPlayerBuGang(m_nActIdx, m_nCard);
			break;
		case eMJAct_Hu:
		{
			if (m_vHuIdxs.empty())
			{
				m_vHuIdxs.push_back(m_nActIdx);
			}
			pRoom->onPlayerHu(m_vHuIdxs, m_nCard, m_nInvokeIdx);
		}
		break;
		case eMJAct_Chu:
			pRoom->onPlayerChu(m_nActIdx, m_nCard);
			break;
		case eMJAct_Chi:
			if (m_vEatWith[0] * m_vEatWith[1] == 0)
			{
				LOGFMTE("eat lack of right card");
				break;
			}
			pRoom->onPlayerEat(m_nActIdx, m_nCard, m_vEatWith[0], m_vEatWith[1], m_nInvokeIdx);
			break;
		default:
			LOGFMTE("unknow act  how to do it %u", m_eActType);
			break;
		}
	}

	float getActTime() override
	{
		switch (m_eActType)
		{
		case eMJAct_Mo:
			return eTime_DoPlayerMoPai;
		case eMJAct_Peng:
		case eMJAct_Chi:
			return eTime_DoPlayerAct_Peng;
		case eMJAct_MingGang:
		case eMJAct_MingGang_Pre:
		case eMJAct_BuGang:
		case eMJAct_AnGang:
		case eMJAct_BuHua:
		case eMJAct_HuaGang:
		case eMJAct_Cyclone:
			return eTime_DoPlayerAct_Gang;
		case eMJAct_Hu:
			return eTime_DoPlayerAct_Hu;
			break;
		case eMJAct_Chu:
			return eTime_DoPlayerActChuPai;
		default:
			LOGFMTE("unknown act type = %u can not return act time", m_eActType);
			return 0;
		}
		return 0;
	}
};