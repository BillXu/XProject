#pragma once
#include "IGameRoomState.h"
class ThirteenRoomStateGameEnd
	:public IGameRoomState
{
public:
	uint32_t getStateID() { return eRoomState_GameEnd; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		getRoom()->onGameEnd();
		setStateDuringTime(eTime_GameOver * getRoom()->getPlayerCnt() * 15);
		//LOGFMTE("enter game end, during = %f", getStateDuring());
		/*if (((ThirteenRoom*)getRoom())->isRoomGameOver()) {
			setStateDuringTime(eTime_GameOver * 30);
		}
		else {
			setStateDuringTime(eTime_GameOver);
		}*/
	}

	/*void update(float fDeta)override
	{
		auto pRoom = (ThirteenRoom*)getRoom();
		if (pRoom->canStartGame())
		{
			pRoom->goToState(eRoomSate_WaitReady);
			return;
		}
		IGameRoomState::update(fDeta);
	}*/

	void onStateTimeUp()
	{
		//LOGFMTE("go out game end");
		auto pRoom = (ThirteenRoom*)getRoom();
		pRoom->onGameDidEnd();
		getRoom()->goToState(eRoomSate_WaitReady);
	}

	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override {
		if (MSG_ROOM_THIRTEEN_CLIENT_OVER == nMsgType) {
			auto pPlayer = getRoom()->getPlayerBySessionID(nSessionID);
			if (pPlayer) {
				setStateDuringTime(0);
			}
			return true;
		}
		return false;
	}
};