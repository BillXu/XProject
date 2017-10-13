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
		setStateDuringTime(eTime_WaitRobotBanker);
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
		
		if ( pRoom->isAllPlayerRobotedBanker() )
		{
			setStateDuringTime(0);
		}
		return true;
	}

	void onStateTimeUp()
	{
		auto pRoom = (NNRoom*)getRoom();
		pRoom->doProduceNewBanker();
		getRoom()->goToState(eRoomState_DoBet);
	}

	uint8_t getCurIdx()override { return m_nNewBankerIdx; };
protected:
	uint8_t m_nNewBankerIdx;
};