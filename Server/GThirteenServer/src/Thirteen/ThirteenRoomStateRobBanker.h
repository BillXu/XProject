#pragma once
#include "IGameRoomState.h"
class ThirteenRoomStateRobBanker
	:public IGameRoomState
{
public:
	uint32_t getStateID()override { return eRoomState_RobotBanker; }

	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)override
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		setStateDuringTime(10);
		Json::Value jsMsg;
		getRoom()->sendRoomMsg(jsMsg, MSG_ROOM_THIRTEEN_START_ROT_BANKER);
	}

	void onStateTimeUp()override
	{
		getRoom()->goToState(eRoomState_DistributeCard);
	}

	void update(float fDeta)override
	{
		IGameRoomState::update(fDeta);
		auto pRoom = (ThirteenRoom*)getRoom();
		if (pRoom->hasRotBanker())
		{
			pRoom->goToState(eRoomState_DistributeCard);
		}
	}

	bool onMsg(Json::Value& jsmsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID) override
	{
		if (MSG_ROOM_THIRTEEN_ROT_BANKER == nMsgType) {
			auto pRoom = (ThirteenRoom*)getRoom();
			auto pPlayer = pRoom->getPlayerBySessionID(nSessionID);
			Json::Value jsRet;
			if (pPlayer == nullptr)
			{
				jsRet["ret"] = 1;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("you are not in this room how to rot banker session id = %u", nSessionID);
				return true;
			}
			pRoom->onPlayerRotBanker(pPlayer->getIdx());
			return true;
		}
		return false;
	}
};