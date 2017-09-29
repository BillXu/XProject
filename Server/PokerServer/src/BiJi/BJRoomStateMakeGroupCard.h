#pragma once
#include "IGameRoomState.h"
#include "BJRoom.h"
#include "IGamePlayer.h"
class BJRoomStateMakeGroup
	:public IGameRoomState
{
public:
	uint32_t getStateID() { return eRoomState_BJ_Make_Group; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		setStateDuringTime(60);
	}

	bool onMsg(Json::Value& jsmsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)
	{
		if ( MSG_PLAYER_MAKED_GROUP != nMsgType )
		{
			return false;
		}

		auto pRoom = (BJRoom*)getRoom();
		auto pPlayer = pRoom->getPlayerBySessionID(nSessionID);
		Json::Value js;
		if (!pPlayer)
		{
			js["ret"] = 1;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		auto& jsCards = jsmsg["vCards"];
		if ( jsCards.size() != 9 )
		{
			js["ret"] = 2;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		// parse cards ;
		std::vector<uint8_t> vGroupCards;
		for (uint8_t nIdx = 0; nIdx < jsCards.size(); ++nIdx)
		{
			vGroupCards.push_back( jsCards[nIdx].asUInt() );
		}

		js["ret"] = pRoom->onPlayerDoMakeCardGroup(pPlayer->getIdx(), vGroupCards);
		pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);

		if ( pRoom->isAllPlayerMakedGroupCard() )
		{
			setStateDuringTime(0);  // all player finished cacualte Niu , process as wait time out ;
		}
		return true;
	}

	void onStateTimeUp()
	{
		auto pRoom = (BJRoom*)getRoom();
		if ( !pRoom->onPlayerAutoMakeCardGroupAllPlayerOk() )
		{
			setStateDuringTime(60);
		}
		else
		{
			getRoom()->goToState(eRoomState_GameEnd);
		}
	}
};