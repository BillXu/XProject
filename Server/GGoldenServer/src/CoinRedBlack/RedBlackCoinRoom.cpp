#include "RedBlackCoinRoom.h"
#include "RedBlackRoom.h"
#include "RedBlackPlayer.h"
#include "IGameRoomManager.h"
#include "ISeverApp.h"
#include "AsyncRequestQuene.h"
void RedBlackCoinRoom::onStartGame(IGameRoom* pRoom)
{
	// nothing to do 
}

bool RedBlackCoinRoom::onPlayerNetStateRefreshed(uint32_t nPlayerID, eNetState nState)
{
	auto pp = (RedBlackPlayer*)getCoreRoom()->getPlayerByUID(nPlayerID);
	if ( eNet_Offline == nState && pp )
	{
		if (pp->getBetCoin() == 0 && ((RedBlackRoom*)getCoreRoom())->getBankerUID() != nPlayerID)
		{
			getCoreRoom()->doPlayerLeaveRoom(nPlayerID);
			return true;
		}

		LOGFMTE("player uid = %u do offline , but can not let sit down player leave room , room is started room id = %u", nPlayerID, getRoomID());
		m_vDelayLeave.insert(pp->getUserUID());
		return true;
	}
	return getCoreRoom()->onPlayerNetStateRefreshed(nPlayerID, nState);
}

GameRoom* RedBlackCoinRoom::doCreatRealRoom()
{
	return new RedBlackRoom();
}

bool RedBlackCoinRoom::onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)
{
	switch (nMsgType)
	{
	case MSG_PLAYER_STAND_UP:
	{
		auto pp = getCoreRoom()->getPlayerBySessionID(nSessionID);
		if ( pp )
		{
			getCoreRoom()->doPlayerStandUp(pp->getUserUID());
		}
		else
		{
			prealMsg["ret"] = 2;
			sendMsgToPlayer(prealMsg, nMsgType, nSessionID);
		}
	}
	break;
	case MSG_PLAYER_LEAVE_ROOM:
	{
		auto pp = (RedBlackPlayer*)getCoreRoom()->getPlayerBySessionID(nSessionID);
		if (pp == nullptr)
		{
			prealMsg["ret"] = 4;
			sendMsgToPlayer(prealMsg, nMsgType, nSessionID);
			LOGFMTE( "not in room , how to leave ?" );
			return true;
		}
		if (pp->getBetCoin() == 0 && ((RedBlackRoom*)getCoreRoom())->getBankerUID() != pp->getUserUID() )
		{
			LOGFMTE("uid = %u in room leave ?", pp->getUserUID());
			getCoreRoom()->doPlayerLeaveRoom(pp->getUserUID());
			return true;
		}
		m_vDelayLeave.insert(pp->getUserUID());
		prealMsg["ret"] = 3;
		sendMsgToPlayer(prealMsg, nMsgType, nSessionID); 
		return true;
	}
	break;
	default:
		return ICoinRoom::onMsg(prealMsg, nMsgType, eSenderPort, nSessionID );
	}
	return true;
}

void RedBlackCoinRoom::onPlayerDoLeaved(IGameRoom* pRoom, uint32_t nUserUID)
{
	auto ps = getCoreRoom()->getPlayerByUID(nUserUID);
	if (nullptr == ps)
	{
		LOGFMTE("why player uid = %u stand obj is null , room id = %u??", nUserUID, getRoomID());
		return;
	}

	auto pAsync = getCoreRoom()->getRoomMgr()->getSvrApp()->getAsynReqQueue();
	Json::Value jsReqLeave;
	jsReqLeave["coin"] = ps->getChips();
	jsReqLeave["targetUID"] = nUserUID;
	pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nUserUID, eAsync_SyncPlayerGameInfo, jsReqLeave);

	Json::Value jsLeve;
	jsLeve["ret"] = 0;
	sendMsgToPlayer(jsLeve, MSG_PLAYER_LEAVE_ROOM, ps->getSessionID() );
}

void RedBlackCoinRoom::sendRoomInfo(uint32_t nSessionID)
{
	Json::Value jsRoomInfo;
	packRoomInfo(jsRoomInfo);
	LOGFMTI("send room info game room");
	auto p = getCoreRoom()->getPlayerBySessionID(nSessionID);
	if (p)
	{
		jsRoomInfo["selfCoin"] = p->getChips() - ((RedBlackPlayer*)p)->getBetCoin();
	}
	sendMsgToPlayer(jsRoomInfo, MSG_ROOM_INFO, nSessionID);

	// send players 
	sendRoomPlayersInfo(nSessionID);
}