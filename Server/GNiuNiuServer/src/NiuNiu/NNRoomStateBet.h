#pragma once
#include "IGameRoomState.h"
#include "NNRoom.h"
#include "IGamePlayer.h"
class NNRoomStateDoBet
	:public IGameRoomState
{
public:
	uint32_t getStateID() { return eRoomState_DoBet; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		auto pRoom = (NNRoom*)getRoom();
		pRoom->doStartBet();
		setStateDuringTime(15);
	}

	bool onMsg( Json::Value& jsmsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID )
	{
		if ( MSG_PLAYER_DO_BET != nMsgType)
		{
			return false;
		}

		auto pRoom = (NNRoom*)getRoom();
		auto pPlayer = pRoom->getPlayerBySessionID(nSessionID);
		Json::Value js;
		if ( !pPlayer )
		{
			js["ret"] = 2;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		if (jsmsg["betTimes"].isNull())
		{
			js["ret"] = 1;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		auto nBetTimes = jsmsg["betTimes"].asUInt();
		js["ret"] = pRoom->onPlayerDoBet(pPlayer->getIdx(), (uint8_t)nBetTimes);
		pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);

		if ( pRoom->isAllPlayerDoneBet() )
		{
			setStateDuringTime(0);
		}

		return true;
	}

	void onStateTimeUp()
	{
		auto pRoom = (NNRoom*)getRoom();
		pRoom->onTimeOutPlayerAutoBet();
		Json::Value jsValue;
		getRoom()->goToState(eRoomState_DistributeCard, &jsValue);
	}

	uint8_t getCurIdx()override { return m_nNewBankerIdx; };
protected:
	uint8_t m_nNewBankerIdx;
};