#pragma once
#include "IGameRoomState.h"
#include "NNRoom.h"
#include "IGamePlayer.h"
class NNRoomStateCaculateNiu
	:public IGameRoomState
{
public:
	uint32_t getStateID() { return eRoomState_CaculateNiu; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		setStateDuringTime(eTime_ExeGameStart);
	}

	bool onMsg(Json::Value& jsmsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)
	{
		if ( MSG_PLAYER_CACULATE_NIU != nMsgType)
		{
			return false;
		}

		auto pRoom = (NNRoom*)getRoom();
		auto pPlayer = pRoom->getPlayerBySessionID(nSessionID);
		Json::Value js;
		if (!pPlayer)
		{
			js["ret"] = 1;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		js["ret"] = pRoom->onPlayerDoCacuateNiu(pPlayer->getIdx());
		pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);

		if ( pRoom->isAllPlayerCacualtedNiu() )
		{
			setStateDuringTime(0);  // all player finished cacualte Niu , process as wait time out ;
		}
		return true;
	}

	void onStateTimeUp()
	{
		getRoom()->goToState(eRoomState_GameEnd);
	}

	uint8_t getCurIdx()override { return m_nNewBankerIdx; };
protected:
	uint8_t m_nNewBankerIdx;
};