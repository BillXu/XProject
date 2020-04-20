#pragma once
#include "DDZRoomStateRobotBanker.h"
class DDZRoomStateRobotBanker2
	:public DDZRoomStateRobotBanker
{
public:
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)override
	{
		DDZRoomStateRobotBanker::enterState(pmjRoom, jsTranData);
		m_nHasRotedTimes = 0;
	}

	bool onMsg(Json::Value& jsmsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override
	{
		if (MSG_DDZ_PLAYER_ROBOT_DZ != nMsgType)
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

		if (nRobotBankerTimes)
		{
			m_nCurMaxRobotTimes = m_nCurMaxRobotTimes ? m_nCurMaxRobotTimes * 2 : 1;
			m_nBankerCandiate = pPlayer->getIdx();
			jsmsg["times"] = m_nCurMaxRobotTimes;
			nRobotBankerTimes = m_nCurMaxRobotTimes;
			m_nHasRotedTimes++;
		}

		m_vMapPlayerIdx_RobotTimes[pPlayer->getIdx()] = nRobotBankerTimes;

		// tell all players 
		jsmsg["idx"] = pPlayer->getIdx();
		pRoom->sendRoomMsg(jsmsg, MSG_DDZ_ROOM_ROBOT_DZ);

		// add frame 
		Json::Value jsFrame;
		jsFrame["idx"] = pPlayer->getIdx();
		jsFrame["times"] = nRobotBankerTimes;
		pRoom->addReplayFrame(DDZ_Frame_DoRobBanker, jsFrame);

		auto bFinishRot = !goOnWaitNextPlayerRotBanker();

		if (bFinishRot) {
			doProduceBanker();
		}
	}

protected:
	bool goOnWaitNextPlayerRotBanker() override
	{
		if (m_nHasRotedTimes > 3) {
			return false;
		}

		for (auto& ref : m_vMapPlayerIdx_RobotTimes) {
			if (ref.second == 0) {
				return false;
			}
		}

		m_nCurWaitPlayerIdx = (m_nCurWaitPlayerIdx + 1) % getRoom()->getSeatCnt();
		setStateDuringTime(eTime_WaitForever);

		Json::Value jsInfo;
		jsInfo["idx"] = m_nCurWaitPlayerIdx;
		getRoom()->sendRoomMsg(jsInfo, MSG_DDZ_ROOM_WAIT_ROBOT_DZ);
		return true;
	}

protected:
	uint8_t m_nHasRotedTimes = 0;
};