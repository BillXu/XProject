#pragma once
#include "IGameRoomState.h"
#include "DDZRoom.h"
#include "IGamePlayer.h"
#include "DDZPlayer.h"
class JJDDZRoomStateChaoZhuang
	:public IGameRoomState
{
public:
	uint32_t getStateID()final { return eRoomState_JJ_DDZ_Chao_Zhuang; }

	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		setStateDuringTime(eTime_WaitForever);
		m_vChosedWhetherChaoZhuang.clear();

		Json::Value jsMsg;
		getRoom()->sendRoomMsg(jsMsg,MSG_DDZ_WAIT_PLAYER_CHAO_ZHUANG);
	}

	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override
	{
		if (MSG_DDZ_PLAYER_CHAO_ZHUANG != nMsgType)
		{
			return false;
		}

		uint8_t nRet = 0;
		auto pPlayer = (DDZPlayer*)getRoom()->getPlayerBySessionID(nSessionID);
		do
		{
			if (pPlayer == nullptr)
			{
				nRet = 1;
				break;
			}

			if (prealMsg["isChao"].isNull() || prealMsg["isChao"].isInt() == false)
			{
				nRet = 3;
				break;
			}

			auto iter = std::find(m_vChosedWhetherChaoZhuang.begin(), m_vChosedWhetherChaoZhuang.end(), pPlayer->getIdx());
			if (iter != m_vChosedWhetherChaoZhuang.end())
			{
				nRet = 2;
				break;
			}
			m_vChosedWhetherChaoZhuang.push_back(pPlayer->getIdx());
			auto isChao = prealMsg["isChao"].asUInt() == 1;
			if (!isChao)
			{
				break;
			}

			// this player do chao ;
			pPlayer->doChaoZhuang();
		} while (0);

		if (nRet)
		{
			prealMsg["ret"] = nRet;
			getRoom()->sendMsgToPlayer(prealMsg, nMsgType, nSessionID);
			return true;
		}

		prealMsg["idx"] = pPlayer->getIdx();
		getRoom()->sendRoomMsg(prealMsg, MSG_DDZ_ROOM_CHAO_ZHUANG);
		if (m_vChosedWhetherChaoZhuang.size() == getRoom()->getSeatCnt() )
		{
			getRoom()->goToState(eRoomState_StartGame);
		}
		return true;
	}

	void roomInfoVisitor(Json::Value& js)override
	{
		IGameRoomState::roomInfoVisitor(js);

		Json::Value jsWaitChaoIdxs;
		for (auto& ref : m_vChosedWhetherChaoZhuang )
		{
			jsWaitChaoIdxs[jsWaitChaoIdxs.size()] = ref;
		}
		js["chosed"] = jsWaitChaoIdxs;
	}

protected:
	std::vector<uint8_t> m_vChosedWhetherChaoZhuang;
};