#pragma once
#include "MJRoomStateDoPlayerAct.h"
#include "YZMJRoom.h"
class YZMJRoomStateDoPlayerAct
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
			getRoom()->goToState(eRoomState_WaitPlayerChu, &jsValue);
		}
		break;
		case eMJAct_Mo:
		{
			Json::Value jsValue;
			jsValue["idx"] = m_nActIdx;
			getRoom()->goToState(eRoomState_WaitPlayerAct, &jsValue);
		}
		break;
		case eMJAct_BuGang:
		case eMJAct_BuGang_Declare:
		case eMJAct_AnGang:
		case eMJAct_MingGang:
		{
			if (((YZMJRoom*)getRoom())->isInternalShouldCloseAll())
			{
				getRoom()->goToState(eRoomState_GameEnd);
			}
			else {
				Json::Value jsValue;
				jsValue["idx"] = m_nActIdx;
				getRoom()->goToState(eRoomState_WaitPlayerAct, &jsValue);
			}
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
};