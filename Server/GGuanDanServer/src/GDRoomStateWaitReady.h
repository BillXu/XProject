#pragma once
#include "IGameRoomState.h"
#include "GDRoom.h"
#include "IGamePlayer.h"
class GDRoomStateWaitReady
	:public IGameRoomState
{
public:
	uint32_t getStateID()final { return eRoomSate_WaitReady; }

	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		setStateDuringTime(eTime_WaitForever);
	}

	void update(float fDeta)override
	{
		IGameRoomState::update(fDeta);
		auto pRoom = getRoom();
		if (pRoom->canStartGame())
		{
			pRoom->goToState(eRoomState_StartGame);
		}
	}

	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override
	{
		if (MSG_PLAYER_SET_READY == nMsgType)
		{
			auto pPlayer = getRoom()->getPlayerBySessionID(nSessionID);
			Json::Value jsRet;
			if (pPlayer == nullptr)
			{
				jsRet["ret"] = 1;
				getRoom()->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("you are not in this room how to set ready ? session id = %u", nSessionID);
				return true;
			}

			if (pPlayer->haveState(eRoomPeer_WaitNextGame) == false)
			{
				jsRet["ret"] = 2;
				jsRet["curState"] = pPlayer->getState();
				getRoom()->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("player state error uid = %u , state = %u", pPlayer->getUserUID(), pPlayer->getState());
				return true;
			}

			pPlayer->setState(eRoomPeer_Ready);
			jsRet["ret"] = 0;
			getRoom()->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			// msg ;
			Json::Value jsMsg;
			jsMsg["idx"] = pPlayer->getIdx();
			getRoom()->sendRoomMsg(jsMsg, MSG_ROOM_PLAYER_READY);
			return true;
		}
		return false;
	}
};