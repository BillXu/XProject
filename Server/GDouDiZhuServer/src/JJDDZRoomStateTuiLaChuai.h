#pragma once
#include "IGameRoomState.h"
#include "DDZRoom.h"
#include "IGamePlayer.h"
class JJDDZRoomStateTiLaChuai
	:public IGameRoomState
{
public:
	uint32_t getStateID()final { return eRoomState_JJ_DDZ_Ti_La_Chuai; }

	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		setStateDuringTime(eTime_WaitPlayerReady);
		m_vWaitChaoZhuang.clear();
		m_isBankerMakedDecide = false;
		
		uint8_t nBankerIdx = ((DDZRoom*)getRoom())->getBankerIdx();
		for (uint8_t nIdx = 0; nIdx < getRoom()->getSeatCnt(); ++nIdx)
		{
			if ( nIdx != nBankerIdx)
			{
				m_vWaitChaoZhuang.push_back(nIdx);
				
				auto p = getRoom()->getPlayerByIdx(nIdx);
				if (!p)
				{
					continue;
				}

				Json::Value jsMsg;
				getRoom()->sendMsgToPlayer(jsMsg, MSG_DDZ_WAIT_PLAYER_CHAO_ZHUANG,p->getSessionID() );
			}
		}
	}
 
	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override
	{
		if ( MSG_DDZ_PLAYER_CHAO_ZHUANG != nMsgType )
		{
			return false;
		}

		uint8_t nRet = 0;
		do 
		{
			auto pPlayer = getRoom()->getPlayerBySessionID(nSessionID);
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
			
			auto iter = std::find(m_vWaitChaoZhuang.begin(),m_vWaitChaoZhuang.end(),pPlayer->getIdx() );
			if (iter == m_vWaitChaoZhuang.end())
			{
				nRet = 2;
				break;
			}
			m_vWaitChaoZhuang.erase(iter);
			auto isChao = prealMsg["isChao"].asUInt() == 1;
			if (!isChao)
			{
				break;
			}

			// this player do chao ;
			pPlayer->addState(eRoomPeer_ChaoZhuang);
			if (false == m_isBankerMakedDecide )
			{
				uint8_t nBankerIdx = ((DDZRoom*)getRoom())->getBankerIdx();
				m_vWaitChaoZhuang.push_back(nBankerIdx);
				m_isBankerMakedDecide = true;

				auto p = getRoom()->getPlayerByIdx(nBankerIdx);
				if (p == nullptr)
				{
					LOGFMTE( "why banker is null £¿" );
					nRet = 1;
					break;
				}
				setStateDuringTime(eTime_WaitPlayerReady);
				Json::Value jsMsg;
				getRoom()->sendMsgToPlayer(jsMsg, MSG_DDZ_WAIT_PLAYER_CHAO_ZHUANG, p->getSessionID() );
			}
		} while ( 0 );
		
		if ( nRet )
		{
			prealMsg["ret"] = nRet;
			getRoom()->sendMsgToPlayer(prealMsg, nMsgType, nSessionID);
			return true;
		}
		
		if ( m_vWaitChaoZhuang.empty())
		{
			getRoom()->goToState(eRoomState_DDZ_Chu); 
		}
		return true;
	}

	void onStateTimeUp()override
	{
		getRoom()->goToState(eRoomState_DDZ_Chu);
	}

	void roomInfoVisitor(Json::Value& js)override
	{
		IGameRoomState::roomInfoVisitor(js);

		Json::Value jsWaitChaoIdxs;
		for (auto& ref : m_vWaitChaoZhuang )
		{
			jsWaitChaoIdxs[jsWaitChaoIdxs.size()] = ref;
		}
		js["waitChaoPlayers"] = jsWaitChaoIdxs;
	}

protected:
	std::vector<uint8_t> m_vWaitChaoZhuang;
	bool m_isBankerMakedDecide;
};