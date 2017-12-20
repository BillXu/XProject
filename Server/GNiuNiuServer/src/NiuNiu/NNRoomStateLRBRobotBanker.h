#pragma once
#include "IGameRoomState.h"
#include "NNRoom.h"
#include "IGamePlayer.h"
class NNRoomStateLRBRobotBanker
	:public IGameRoomState
{
public:
	uint32_t getStateID() { return eRoomState_RobotBanker; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		auto pRoom = (NNRoom*)getRoom();
		pRoom->doStartRobotBanker();
		setStateDuringTime(999999999);
		m_isCheckTuoGuan = false;
		m_isAnimateion = false;
	}

	bool onMsg(Json::Value& jsmsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)
	{
		if ( MSG_PLAYER_ROBOT_BANKER != nMsgType )
		{
			return false;
		}

		auto pRoom = (NNRoom*)getRoom();
		auto pPlayer = pRoom->getPlayerBySessionID(nSessionID);
		Json::Value js;
		if (!pPlayer)
		{
			js["ret"] = 2;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		if (jsmsg["robotTimes"].isNull())
		{
			js["ret"] = 1;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		auto nRobotBankerTimes = jsmsg["robotTimes"].asUInt();
		js["ret"] = pRoom->onPlayerRobotBanker(pPlayer->getIdx(), (uint8_t)nRobotBankerTimes );
		pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
		return true;
	}

	void update(float fDeta)override
	{
		IGameRoomState::update(fDeta);
		auto pRoom = (NNRoom*)getRoom();
		if ( false == m_isAnimateion && pRoom->isAllPlayerRobotedBanker())
		{
			m_isAnimateion = true;
			auto nCandianateCnt = pRoom->doProduceNewBanker();
			float fT = nCandianateCnt * 0.5;
			setStateDuringTime(((uint8_t)fT) > 2 ? 2 : fT);
		}

		if (false == m_isCheckTuoGuan)
		{
			m_isCheckTuoGuan = true;
			pRoom->invokerTuoGuanAction();
		}
	}

	void onStateTimeUp()
	{
		getRoom()->goToState(eRoomState_DoBet);
	}

protected:
	bool m_isCheckTuoGuan;
	bool m_isAnimateion;
};