#pragma once
#include "IGameRoomState.h"
#include "DDZRoom.h"
#include "IGamePlayer.h"
#include "DDZPlayer.h"
class JJDDZRoomStateTiLaChuai
	:public IGameRoomState
{
public:
	uint32_t getStateID()final { return eRoomState_JJ_DDZ_Ti_La_Chuai; }

	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		setStateDuringTime(eTime_WaitForever);
		m_vWaitChaoZhuang.clear();
		m_isBankerMakedDecide = false;
		
		uint8_t nBankerIdx = ((DDZRoom*)getRoom())->getBankerIdx();

		if (jsTranData.isArray() == false)
		{
			LOGFMTE( "ti la chuai  must have info " );
		}

		std::map<uint8_t, uint8_t> vPlayerRobTimes;
		for ( uint8_t nIdx = 0; nIdx < jsTranData.size(); ++nIdx)
		{
			auto jsi = jsTranData[nIdx];
			vPlayerRobTimes[jsi["idx"].asUInt()] = jsi["times"].asUInt();
		}

		Json::Value jsWaitTiLaChuai;
		for ( uint8_t nIdx = 0; nIdx < getRoom()->getSeatCnt(); ++nIdx)
		{
			if (nIdx == nBankerIdx)
			{
				continue;
			}

			auto iter = vPlayerRobTimes.find(nIdx);
			if (iter == vPlayerRobTimes.end() || iter->second != 0 )
			{
				jsWaitTiLaChuai[jsWaitTiLaChuai.size()] = nIdx;
				m_vWaitChaoZhuang.push_back(nIdx);
			}
		}

		if (m_vWaitChaoZhuang.empty())
		{
			setStateDuringTime(0.0001); // right time out ;
			return;
		}
		Json::Value jsMsg;
		jsMsg["waitTiLaChuaiPlayers"] = jsWaitTiLaChuai;
		getRoom()->sendRoomMsg(jsMsg, MSG_DDZ_WAIT_PLAYER_TI_LA_CHUAI);
	}
 
	void onStateTimeUp()override
	{
		getRoom()->goToState(eRoomState_StartGame);
	}

	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override
	{
		if (MSG_DDZ_PLAYER_TI_LA_CHUAI != nMsgType )
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

			if (prealMsg["isTiLaChuai"].isNull() || prealMsg["isTiLaChuai"].isInt() == false)
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
			auto isChao = prealMsg["isTiLaChuai"].asUInt() == 1;
			if (!isChao)
			{
				break;
			}

			// this player do chao ;
			pPlayer->doTiLaChuai();
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
			}
		} while ( 0 );
		
		if ( nRet )
		{
			prealMsg["ret"] = nRet;
			getRoom()->sendMsgToPlayer(prealMsg, nMsgType, nSessionID);
			return true;
		}
		
		prealMsg["idx"] = pPlayer->getIdx();
		getRoom()->sendRoomMsg(prealMsg, MSG_DDZ_ROOM_TI_LA_CHUAI );

		if ( m_vWaitChaoZhuang.empty())
		{
			getRoom()->goToState(eRoomState_StartGame);
		}
		return true;
	}

	void roomInfoVisitor(Json::Value& js)override
	{
		IGameRoomState::roomInfoVisitor(js);

		Json::Value jsWaitChaoIdxs;
		for (auto& ref : m_vWaitChaoZhuang )
		{
			jsWaitChaoIdxs[jsWaitChaoIdxs.size()] = ref;
		}
		js["waitTiLaChuaiPlayers"] = jsWaitChaoIdxs;
	}

protected:
	std::vector<uint8_t> m_vWaitChaoZhuang;
	bool m_isBankerMakedDecide;
};