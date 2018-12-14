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
		m_nBankerCandiate = m_nCurWaitPlayerIdx;
		// send msg tell room player ;
		setStateDuringTime(eTime_WaitForever);

		Json::Value jsInfo;
		jsInfo["idx"] = m_nCurWaitPlayerIdx;
		getRoom()->sendRoomMsg(jsInfo, MSG_DDZ_ROOM_WAIT_ROBOT_DZ );

		// add frame 
		getRoom()->addReplayFrame(DDZ_Frame_WaitRobBanker, jsInfo);
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

		if (nRobotBankerTimes == 0 && isPlayerMustRobBanker(pPlayer->getIdx()))
		{
			js["ret"] = 4;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		if (eFALRLT_Call == pRoom->getRLT()) {
			if (nRobotBankerTimes <= m_nCurMaxRobotTimes && nRobotBankerTimes != 0)
			{
				js["ret"] = 4;
				pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
				return true;
			}

			// tell all players 
			jsmsg["idx"] = pPlayer->getIdx();
			pRoom->sendRoomMsg(jsmsg, MSG_DDZ_ROOM_ROBOT_DZ);

			// add frame 
			Json::Value jsFrame;
			jsFrame["idx"] = pPlayer->getIdx();
			jsFrame["times"] = nRobotBankerTimes;
			pRoom->addReplayFrame(DDZ_Frame_DoRobBanker, jsFrame);

			if (nRobotBankerTimes != 0)
			{
				m_nCurMaxRobotTimes = nRobotBankerTimes;
				m_nBankerCandiate = pPlayer->getIdx();
			}
			m_vMapPlayerIdx_RobotTimes[pPlayer->getIdx()] = nRobotBankerTimes;
			if (getMaxBankerTimes() == nRobotBankerTimes || m_vMapPlayerIdx_RobotTimes.size() == pRoom->getSeatCnt())
			{
				doProduceBanker();
			}
			else
			{
				// inform next player robot di zhu 
				goOnWaitNextPlayerRobotBanker();
			}
		}
		else {
			if (nRobotBankerTimes)
			{
				m_nCurMaxRobotTimes = m_nCurMaxRobotTimes ? m_nCurMaxRobotTimes * 2 : 1;
				m_nBankerCandiate = pPlayer->getIdx();
				jsmsg["times"] = m_nCurMaxRobotTimes;
				nRobotBankerTimes = m_nCurMaxRobotTimes;
			}

			//LOGFMTE("player uid = %u rot banker time = %u", pPlayer->getIdx(), nRobotBankerTimes);
			
			bool bFinishRot = m_vMapPlayerIdx_RobotTimes.count(pPlayer->getIdx());
			m_vMapPlayerIdx_RobotTimes[pPlayer->getIdx()] = nRobotBankerTimes;
			

			// tell all players 
			jsmsg["idx"] = pPlayer->getIdx();
			pRoom->sendRoomMsg(jsmsg, MSG_DDZ_ROOM_ROBOT_DZ);

			// add frame 
			Json::Value jsFrame;
			jsFrame["idx"] = pPlayer->getIdx();
			jsFrame["times"] = nRobotBankerTimes;
			pRoom->addReplayFrame(DDZ_Frame_DoRobBanker, jsFrame);

			if (bFinishRot) {
				//LOGFMTE("player uid = %u rot banker time = %u finish true finish", pPlayer->getIdx(), nRobotBankerTimes);
				doProduceBanker();
			}
			else
			{
				// inform next player robot di zhu 
				if (goOnWaitNextPlayerRotBanker() == false) {
					//LOGFMTE("player uid = %u rot banker time = %u finish false finish", pPlayer->getIdx(), nRobotBankerTimes);
					doProduceBanker();
				}
			}
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
		setStateDuringTime(eTime_WaitForever);

		Json::Value jsInfo;
		jsInfo["idx"] = m_nCurWaitPlayerIdx;
		getRoom()->sendRoomMsg(jsInfo, MSG_DDZ_ROOM_WAIT_ROBOT_DZ);
		return true;
	}

	bool goOnWaitNextPlayerRotBanker()
	{
		bool flag = false;
		uint8_t nSeatCnt = getRoom()->getSeatCnt();
		uint8_t nCheckIdx = -1;
		for (uint8_t i = 1; i < nSeatCnt; i++) {
			uint8_t tCheckIdx = (m_nCurWaitPlayerIdx + i) % nSeatCnt;
			if (m_vMapPlayerIdx_RobotTimes.count(tCheckIdx)) {
				if (m_vMapPlayerIdx_RobotTimes[tCheckIdx]) {
					if (m_vMapPlayerIdx_RobotTimes[tCheckIdx] > 1) {
						break;
					}
					else {
						nCheckIdx = tCheckIdx;
						break;
					}
				}
				else {
					continue;
				}
			}
			else {
				m_nCurWaitPlayerIdx = tCheckIdx;
				flag = true;
				break;
			}
		}

		if ((uint8_t)-1 != nCheckIdx) {
			for (auto& ref : m_vMapPlayerIdx_RobotTimes) {
				if (ref.first != nCheckIdx && ref.second) {
					m_nCurWaitPlayerIdx = nCheckIdx;
					flag = true;
					break;
				}
			}
		}

		if (flag) {
			// send msg tell room player ;
			setStateDuringTime(eTime_WaitForever);

			Json::Value jsInfo;
			jsInfo["idx"] = m_nCurWaitPlayerIdx;
			getRoom()->sendRoomMsg(jsInfo, MSG_DDZ_ROOM_WAIT_ROBOT_DZ);
		}
		return flag;
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
			m_nBankerCandiate = pRoom->getFirstRobotBankerIdx();
		}

		auto pBanker = (DDZPlayer*)pRoom->getPlayerByIdx(m_nBankerCandiate);
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

		// add frame 
		getRoom()->addReplayFrame(DDZ_Frame_DoProducedBanker, jsMsg);
		// go to chu pai state ;
		/*Json::Value jsRobotBankerInfo;
		for ( auto& ref : m_vMapPlayerIdx_RobotTimes )
		{
			Json::Value jsItem;
			jsItem["idx"] = ref.first;
			jsItem["times"] = ref.second;
			jsRobotBankerInfo[jsRobotBankerInfo.size()] = jsItem;
		}*/
		//getRoom()->goToState( isJingJiangDDZ() ? eRoomState_JJ_DDZ_Ti_La_Chuai :  eRoomState_DDZ_Chu, &jsRobotBankerInfo);
		getRoom()->goToState(eRoomState_DDZ_Double);
	}

	bool isPlayerMustRobBanker( uint8_t nIdx )
	{
		if (isCYDDZ()) {
			return false;
		}

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

	bool isCYDDZ()
	{
		return eGame_CYDouDiZhu == getRoom()->getRoomType();
	}

	uint8_t getMaxBankerTimes()
	{
		return isJingJiangDDZ() ? 1 : 3;
	}
protected:
	std::map<uint8_t, uint8_t> m_vMapPlayerIdx_RobotTimes;
	uint8_t m_nCurWaitPlayerIdx;
	uint8_t m_nBankerCandiate;
	uint8_t m_nCurMaxRobotTimes;
};