#pragma once
#include "IGameRoomState.h"
#include "CommonDefine.h"
#include "RedBlackRoom.h"
#include "RedBlackPlayer.h"
class RedBlackRoomStateStartBet
	:public IGameRoomState
{
public:
	uint32_t getStateID()final { return eRoomState_StartGame; }

	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		setStateDuringTime(eTime_RedBlack_WaitBet);
		getRoom()->onWillStartGame();
		getRoom()->onStartGame();
	}

	void onStateTimeUp()
	{
		getRoom()->goToState(eRoomState_GameEnd);
	}

	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override
	{
		if ( MSG_RB_PLAYER_BET == nMsgType )
		{
			// client : { coin : 23 , poolType : 23  }
			// svr : { ret : 0 }
			// poolType : eBetPool ;
			// ret : 0 success , 1 coin not enough , 2 pool is limit , 3 other error 
			auto nBetCoin = prealMsg["coin"].asInt();
			auto nPoolType = prealMsg["poolType"].asInt();
			auto player = (RedBlackPlayer*)getRoom()->getPlayerBySessionID(nSessionID);
			if ( nullptr == player )
			{
				prealMsg["ret"] = 4;
				getRoom()->sendMsgToPlayer(prealMsg, nMsgType, nSessionID );
				return true;
			}

			auto pRedBlackRoom = (RedBlackRoom*)getRoom();
			if (player->getUserUID() == pRedBlackRoom->getBankerUID())
			{
				prealMsg["ret"] = 5;
				getRoom()->sendMsgToPlayer(prealMsg, nMsgType, nSessionID);
				return true;
			}

			if ( pRedBlackRoom->getPoolCapacityToBet((eBetPool)nPoolType) < nBetCoin )
			{
				prealMsg["ret"] = 2;
				getRoom()->sendMsgToPlayer(prealMsg, nMsgType, nSessionID);
				return true;
			}

			if ( player->doBet(nBetCoin, (eBetPool)nPoolType) == false )
			{
				prealMsg["ret"] = 1;
				getRoom()->sendMsgToPlayer(prealMsg, nMsgType, nSessionID);
				return true;
			}

			pRedBlackRoom->getPool((eBetPool)nPoolType).nBetChips += nBetCoin;

			prealMsg["idx"] = player->getIdx();
			prealMsg["uid"] = player->getUserUID();
			getRoom()->sendRoomMsg(prealMsg, MSG_RB_ROOM_BET );
			return true;
		}
		return false;
	}
};
