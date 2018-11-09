#pragma once
#include "MJRoomStateWaitReady.h"
class MJRoomStateAutoStartWaitReady
	:public CMJRoomStateWaitReady
{
public:
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)override
	{
		CMJRoomStateWaitReady::enterState(pmjRoom, jsTranData);
		if (autoStartTime == 0.0f) {
			autoStartTime = 10.0f;
		}
#ifdef _DEBUG
		autoStartTime = 30.0f;
#endif // DEBUG

		if (m_bHasStarted) {
			setStateDuringTime(autoStartTime);
		}
	}

	void update(float fDeta)override
	{
		IGameRoomState::update(fDeta);
		auto pRoom = getRoom();
		if (pRoom->canStartGame())
		{
			setStateDuringTime(0);
			//pRoom->goToState(eRoomState_StartGame);
		}
	}

	void onStateTimeUp()override
	{
		auto pRoom = (IMJRoom*)getRoom();
		pRoom->autoDoPlayerSetReady();
		if (pRoom->canStartGame())
		{
			m_bHasStarted = true;
			pRoom->goToState(eRoomState_StartGame);
		}
		else {
			setStateDuringTime(autoStartTime);
		}
	}

	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override
	{
		if (MSG_PLAYER_SET_READY == nMsgType)
		{
			auto pPlayer = getRoom()->getPlayerBySessionID(nSessionID);
			if (pPlayer == nullptr || (pPlayer->haveState(eRoomPeer_WaitNextGame) == false))
			{
				LOGFMTE("you are not in this room how to set ready ? session id = %u", nSessionID);
				return true;
			}
			((IMJRoom*)getRoom())->onPlayerSetReady((uint8_t)pPlayer->getIdx());
			if (getRoom()->canStartGame())
			{
				setStateDuringTime(0);
			}
			return true;
		}
		return false;
	}

protected:
	float autoStartTime = 0.0f;
	bool m_bHasStarted = false;
};