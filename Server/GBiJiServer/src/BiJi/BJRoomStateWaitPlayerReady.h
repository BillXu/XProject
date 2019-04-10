#pragma once
#include "IGameRoomState.h"
#include "BiJi\BJRoom.h"
#include "IGamePlayer.h"
class BJRoomStateWaitReady
	:public IGameRoomState
{
public:
	uint32_t getStateID()final { return eRoomSate_WaitReady; }

	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
	}

	void onStateTimeUp()
	{
		// all room set ready 
		for (uint8_t nIdx = 0; nIdx < getRoom()->getSeatCnt(); ++nIdx)
		{
			auto ptrPlayer = getRoom()->getPlayerByIdx(nIdx);
			if (ptrPlayer == nullptr || ptrPlayer->haveState(eRoomPeer_WaitNextGame) == false)
			{
				continue;
			}
			((BJRoom*)getRoom())->onPlayerReady(nIdx);
			LOGFMTD("auto set ready room id = %u , uid = %u", getRoom()->getRoomID(), ptrPlayer->getUserUID());
		}

		if (getRoom()->canStartGame())
		{
			getRoom()->goToState(eRoomState_StartGame);
		}
		else
		{
			//setStateDuringTime(eTime_WaitPlayerReady);
			setStateDuringTime(15);
		}
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

			((BJRoom*)getRoom())->onPlayerReady(pPlayer->getIdx());
			jsRet["ret"] = 0;
			getRoom()->sendMsgToPlayer(jsRet, nMsgType, nSessionID);

			if (getRoom()->canStartGame())
			{
				getRoom()->goToState(eRoomState_StartGame);
			}
			return true;
		}
		return false;
	}
};