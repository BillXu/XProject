#pragma once
#include "IGameRoomState.h"
#include "DDZRoom.h"
#include "DDZPlayer.h"
#include "IPoker.h"
class DDZRoomStateRobotBanker
	:public IGameRoomState
{
public:
	uint32_t getStateID() { return eRoomState_RobotBanker; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		m_vMapPlayerIdx_RobotTimes.clear();
		m_nCurMaxRobotTimes = 0;

		auto pRoom = (DDZRoom*)getRoom();
		m_nCurWaitPlayerIdx = pRoom->getFirstRobotBankerIdx();
		// send msg tell room player ;
		setStateDuringTime(eTime_WaitRobotBanker);

		Json::Value jsInfo;
		jsInfo["idx"] = m_nCurWaitPlayerIdx;
		getRoom()->sendRoomMsg(jsInfo, MSG_DDZ_ROOM_WAIT_ROBOT_DZ );
	}

	bool onMsg(Json::Value& jsmsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)
	{
		if ( MSG_DDZ_PLAYER_ROBOT_DZ != nMsgType)
		{
			return false;
		}

		auto pRoom = (DDZRoom*)getRoom();
		auto pPlayer = (DDZPlayer*)pRoom->getPlayerBySessionID(nSessionID);
		Json::Value js;
		if (!pPlayer)
		{
			js["ret"] = 2;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		if (jsmsg["times"].isNull())
		{
			js["ret"] = 1;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		if (m_nCurWaitPlayerIdx != pPlayer->getIdx())
		{
			js["ret"] = 3;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		auto nRobotBankerTimes = jsmsg["times"].asUInt();
		if (nRobotBankerTimes <= m_nCurMaxRobotTimes && nRobotBankerTimes != 0 )
		{
			js["ret"] = 4;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		if ( nRobotBankerTimes == 0 && isPlayerMustRobBanker(pPlayer->getIdx()) )
		{
			js["ret"] = 4;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		// tell all players 
		jsmsg["idx"] = pPlayer->getIdx();
		pRoom->sendRoomMsg(jsmsg, MSG_DDZ_ROOM_ROBOT_DZ );

		if (nRobotBankerTimes != 0)
		{
			m_nCurMaxRobotTimes = nRobotBankerTimes;
		}
		m_vMapPlayerIdx_RobotTimes[pPlayer->getIdx()] = nRobotBankerTimes;
		if ( 3 == nRobotBankerTimes || m_vMapPlayerIdx_RobotTimes.size() == getRoom()->getSeatCnt() )
		{
			doProduceBanker();
		}
		else
		{
			// inform next player robot di zhu 
			goOnWaitNextPlayerRobotBanker();
		}
		return true;
	}

	void onStateTimeUp()
	{
		auto pRoom = (DDZRoom*)getRoom();
		auto p = pRoom->getPlayerByIdx(m_nCurWaitPlayerIdx);
		if ( p == nullptr ) 
		{
			LOGFMTE( "why cur wait player robot banker is valid idx = %u , room id = %u",m_nCurWaitPlayerIdx,pRoom->getRoomID() );
			doProduceBanker();
			return;
		}

		// auto robot banker , give up ;
		Json::Value jsmsg;
		jsmsg["robotTimes"] = 0;
		onMsg(jsmsg, MSG_PLAYER_ROBOT_BANKER,ID_MSG_PORT_CLIENT,p->getSessionID());
		return;
	}
	void roomInfoVisitor(Json::Value& js)override
	{
		IGameRoomState::roomInfoVisitor(js);
		js["curActIdx"] = m_nCurWaitPlayerIdx;
		js["curMaxTimes"] = m_nCurMaxRobotTimes;

		Json::Value jsAlreadyRobots;
		for (auto& ref : m_vMapPlayerIdx_RobotTimes)
		{
			Json::Value jsItem;
			jsItem["idx"] = ref.first;
			jsItem["times"] = ref.second;
			jsAlreadyRobots[jsAlreadyRobots.size()] = jsItem;
		}
		js["readyPlayers"] = jsAlreadyRobots;
	}

	uint8_t getCurIdx()override { return m_nCurWaitPlayerIdx; };
protected:
	bool goOnWaitNextPlayerRobotBanker()
	{
		m_nCurWaitPlayerIdx = (++m_nCurWaitPlayerIdx) % getRoom()->getSeatCnt();
		// send msg tell room player ;
		setStateDuringTime(eTime_WaitRobotBanker);

		Json::Value jsInfo;
		jsInfo["idx"] = m_nCurWaitPlayerIdx;
		getRoom()->sendRoomMsg(jsInfo, MSG_DDZ_ROOM_WAIT_ROBOT_DZ);
		return true;
	}

	void doProduceBanker()
	{
		// find banker 
		auto pRoom = (DDZRoom*)getRoom();
		if (0 == m_nCurMaxRobotTimes) // nobody robot Di Zhu  ;
		{
			if (isJingJiangDDZ())
			{
				getRoom()->goToState(eRoomState_StartGame);
				return;
			}

			m_nCurMaxRobotTimes = 1;
			m_nCurWaitPlayerIdx = pRoom->getFirstRobotBankerIdx();
		}

		auto pBanker = (DDZPlayer*)pRoom->getPlayerByIdx(m_nCurWaitPlayerIdx);
		if ( pBanker == nullptr)
		{
			LOGFMTE( "banker is null , why ? room id = %u",pRoom->getRoomID() );
			return;
		}
		// give final card to banker ;
		auto pPoker = getRoom()->getPoker();
		std::vector<uint8_t> vFinal3Cards;
		if (pPoker->getLeftCardCount() != 3)
		{
			LOGFMTE( "dou di zhu room why left card != 3 ? room id = %u",getRoom()->getRoomID() );
			return;
		}
		vFinal3Cards.push_back(pPoker->distributeOneCard());
		vFinal3Cards.push_back(pPoker->distributeOneCard());
		vFinal3Cards.push_back(pPoker->distributeOneCard());
		for (auto& ref : vFinal3Cards)
		{
			pBanker->getPlayerCard()->addHoldCard(ref);
		}
		pRoom->setNewBankerInfo(pBanker->getIdx(),m_nCurMaxRobotTimes,vFinal3Cards);
		
		// send msg tell room player ;
		Json::Value jsMsg;
		jsMsg["dzIdx"] = pRoom->getBankerIdx();
		jsMsg["times"] = pRoom->getBankTimes();
		Json::Value jsCards;
		for (auto& ref : vFinal3Cards)
		{
			jsCards[jsCards.size()] = ref;
		}
		jsMsg["cards"] = jsCards;
		getRoom()->sendRoomMsg(jsMsg, MSG_DDZ_ROOM_PRODUCED_DZ);
		// go to chu pai state ;
		getRoom()->goToState( isJingJiangDDZ() ? eRoomState_JJ_DDZ_Ti_La_Chuai :  eRoomState_DDZ_Chu);
	}

	bool isPlayerMustRobBanker( uint8_t nIdx )
	{
		auto pPlayer = (DDZPlayer*)getRoom()->getPlayerByIdx(nIdx);
		if (!pPlayer)
		{
			return false;
		}
		auto pHoldCards = pPlayer->getPlayerCard();

		// is have joker ?
		if (pHoldCards->isHaveCard(DDZ_MAKE_CARD(ePoker_Joker, 18)) && pHoldCards->isHaveCard(DDZ_MAKE_CARD(ePoker_Joker, 19)))
		{
			return true;
		}

		// is have 4 coutn 2
		for (uint8_t nType = ePoker_None; nType < ePoker_NoJoker; ++nType)
		{
			if ( false == pHoldCards->isHaveCard(DDZ_MAKE_CARD(nType, 16)))
			{
				return false;
			}
		}
		return true;
	}

	bool isJingJiangDDZ()
	{
		return eGame_JJDouDiZhu == getRoom()->getRoomType();
	}
protected:
	std::map<uint8_t, uint8_t> m_vMapPlayerIdx_RobotTimes;
	uint8_t m_nCurWaitPlayerIdx;
	uint8_t m_nCurMaxRobotTimes;
};