#pragma once 
#include "IGameRoomState.h"
#include "IMJRoom.h"
#include "log4z.h"
#include "IMJPlayer.h"
class CMJRoomStateWaitReady
	:public IGameRoomState
{
public:
	uint32_t getStateID()final{ return eRoomSate_WaitReady; }

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
		if ( MSG_PLAYER_SET_READY == nMsgType)
		{
			auto pPlayer = getRoom()->getPlayerBySessionID(nSessionID);
			if (pPlayer == nullptr || (pPlayer->haveState(eRoomPeer_WaitNextGame) == false ))
			{
				LOGFMTE("you are not in this room how to set ready ? session id = %u", nSessionID );
				return true;
			}
			((IMJRoom*)getRoom())->onPlayerSetReady(pPlayer->getIdx());
			if (getRoom()->canStartGame())
			{
				getRoom()->goToState(eRoomState_StartGame);
			}
			return true;
		}
		return false;
	}
};