#include "GameRoom.h"
#include "IGameRoomManager.h"
#include "IGamePlayer.h"
#include "IGameRoomDelegate.h"
#include "log4z.h"
#include "ISeverApp.h"
#include "AsyncRequestQuene.h"
bool GameRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)
{
	m_pDelegate = nullptr;
	m_pRoomMgr = pRoomMgr;
	m_nSieralNum = nSeialNum;
	m_nRoomID = nRoomID;
	m_vPlayers.resize(nSeatCnt);
	m_jsOpts = vJsOpts;
	m_vStandPlayers.clear();
	return true;
}

GameRoom::~GameRoom()
{
	for (auto& ref : m_vPlayers)
	{
		delete ref;
		ref = nullptr;
	}

	for (auto& ref : m_vStandPlayers)
	{
		delete ref.second;
		ref.second = nullptr;
	}
	m_vStandPlayers.clear();
}

void GameRoom::setDelegate(IGameRoomDelegate* pDelegate)
{
	m_pDelegate = pDelegate;
}

uint8_t GameRoom::checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)
{
	if (isRoomFull())
	{
		return 1;
	}
	return 0;
}

bool GameRoom::onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)
{
	// check is already sit down ?
	auto psitDown = getPlayerByUID(pEnterRoomPlayer->nUserUID );
	if ( psitDown )
	{
		LOGFMTE("this player is sitdown uid = %u , room id = %u , why enter room again ?",psitDown->getUserUID(),getRoomID() );
		sendRoomInfo(pEnterRoomPlayer->nSessionID);
		return true;
	}

	auto iterStand = m_vStandPlayers.find( pEnterRoomPlayer->nUserUID );
	if (iterStand == m_vStandPlayers.end())
	{
		auto p = new stStandPlayer();
		p->nSessionID = pEnterRoomPlayer->nSessionID;
		p->nUserUID = pEnterRoomPlayer->nUserUID;
		m_vStandPlayers[p->nUserUID] = p;
		LOGFMTD("room id = %u , player uid = %u enter room chip = %u",getRoomID(),p->nUserUID,pEnterRoomPlayer->nChip );
	}
	else
	{
		LOGFMTE("room id = %u uid = %u already in this room why enter again ?",getRoomID(),pEnterRoomPlayer->nUserUID);
		iterStand->second->nSessionID = pEnterRoomPlayer->nSessionID;
	}
	sendRoomInfo(pEnterRoomPlayer->nSessionID);
	return true;
}

bool GameRoom::isRoomFull()
{
	for (auto& ref : m_vPlayers)
	{
		if (ref == nullptr)
		{
			return false;
		}
	}

	return true;
}

bool GameRoom::doDeleteRoom()
{
	std::vector<uint32_t> vAllInRoomPlayers;
	for (auto& pPayer : m_vPlayers)
	{
		vAllInRoomPlayers.push_back(pPayer->getUserUID());
	}

	for (auto& pStand : m_vStandPlayers)
	{
		vAllInRoomPlayers.push_back(pStand.second->nUserUID );
	}

	for (auto& ref : vAllInRoomPlayers)
	{
		doPlayerLeaveRoom(ref);
	}
	LOGFMTD("room id = %u do delete",getRoomID() );
	return true;
}

void GameRoom::onWillStartGame()
{
	for (auto& ref : m_vPlayers)
	{
		if ( ref )
		{
			ref->onGameWillStart();
		}
	}

	if ( getDelegate() )
	{
		getDelegate()->onWillStartGame(this);
	}
}

void GameRoom::onStartGame()
{
	for (auto& ref : m_vPlayers)
	{
		if (ref)
		{
			ref->onGameStart();
		}
	}
	if (getDelegate())
	{
		getDelegate()->onStartGame(this);
	}
}

bool GameRoom::canStartGame()
{
	if ( getDelegate() && getDelegate()->canStartGame(this) == false )
	{
		return false;
	}
	return true;
}

void GameRoom::onGameDidEnd()
{
	for (auto& ref : m_vPlayers)
	{
		if (ref)
		{
			ref->onGameDidEnd();
		}
	}

	if (getDelegate())
	{
		getDelegate()->onGameDidEnd(this);
	}
}

void GameRoom::onGameEnd()
{
	for (auto& ref : m_vPlayers)
	{
		if (ref)
		{
			ref->onGameEnd();
		}
	}

	if (getDelegate())
	{
		getDelegate()->onGameEnd(this);
	}
}

bool GameRoom::doPlayerSitDown(stEnterRoomData* pEnterRoomPlayer, uint16_t nIdx )
{
	auto pPlayer = getPlayerByIdx(nIdx);
	if (pPlayer)
	{
		LOGFMTE("target pos already have palyer uid = %u , uid = % can not sit idx = %u , room id= %u", pPlayer->getUserUID(), pEnterRoomPlayer->nUserUID, nIdx, getRoomID());
		return false;
	}

	auto pStand = m_vStandPlayers.find(pEnterRoomPlayer->nUserUID);
	if (pStand == m_vStandPlayers.end())
	{
		LOGFMTE("room id = %u , player id = %u , not enter room how to sit down",getRoomID(), pEnterRoomPlayer->nUserUID );
		return false;
	}
	delete pStand->second;
	m_vStandPlayers.erase(pStand);

	auto p = createGamePlayer();
	p->init(pEnterRoomPlayer, nIdx);
	m_vPlayers[p->getIdx()] = p;
	
	Json::Value jsRoomPlayerSitDown;
	visitPlayerInfo(p, jsRoomPlayerSitDown);
	sendRoomMsg(jsRoomPlayerSitDown,MSG_ROOM_SIT_DOWN);
	if (getDelegate())
	{
		getDelegate()->onPlayerSitDown(this,p);
	}
	return true;
}

bool GameRoom::doPlayerStandUp( uint32_t nUserUID )
{
	auto pPlayer = getPlayerByUID(nUserUID);
	if ( nullptr == pPlayer )
	{
		LOGFMTE( "room id = %u palyer uid = %u��not sit down , how to stand up ?",getRoomID(),nUserUID );
		return false;
	}

	auto pStand = m_vStandPlayers.find(nUserUID);
	if ( m_vStandPlayers.end() != pStand )
	{
		LOGFMTE("room id = %u , player is sit down  why have stand up ? uid = %u",getRoomID(),nUserUID );
		return false;
	}

	if (getDelegate())
	{
		getDelegate()->onPlayerWillStandUp(this, pPlayer);
	}

	// send standup msg 
	Json::Value jsMsg;
	jsMsg["idx"] = pPlayer->getIdx();
	jsMsg["uid"] = pPlayer->getUserUID();
	sendRoomMsg(jsMsg,MSG_ROOM_STAND_UP );
	
	// add to standup 
	auto pS = new stStandPlayer();
	pS->nSessionID = pPlayer->getSessionID();
	pS->nUserUID = nUserUID;
	m_vStandPlayers[pS->nUserUID] = pS;

	// do delete player ;
	m_vPlayers[pPlayer->getIdx()] = nullptr;
	delete pPlayer;
	pPlayer = nullptr;

	if (getDelegate())
	{
		getDelegate()->onPlayerStandedUp(this, nUserUID );
	}
	return true;
}

bool GameRoom::doPlayerLeaveRoom(uint32_t nUserUID)
{
	auto pPlayer = getPlayerByUID(nUserUID);
	if (pPlayer)
	{
		doPlayerStandUp(nUserUID);
	}

	auto iterStand = m_vStandPlayers.find( nUserUID );
	if ( m_vStandPlayers.end() == iterStand)
	{
		LOGFMTE("uid = %u not stand in this room = %u how to leave ?",nUserUID,getRoomID());
		return false;
	}

	delete iterStand->second;
	m_vStandPlayers.erase(iterStand);
	if (getDelegate())
	{
		getDelegate()->onPlayerDoLeaved(this,nUserUID );
	}

	// tell data svr player leave ;
	Json::Value jsReqLeave;
	jsReqLeave["targetUID"] = nUserUID;
	jsReqLeave["roomID"] = getRoomID();
	auto pAsync = getRoomMgr()->getSvrApp()->getAsynReqQueue();
	pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nUserUID, eAsync_Inform_Player_LeavedRoom, jsReqLeave);
	return true;
}

uint32_t GameRoom::getRoomID()
{
	return m_nRoomID;
}

uint32_t GameRoom::getSeiralNum()
{
	return m_nSieralNum;
}

void GameRoom::update(float fDelta)
{

}

void GameRoom::sendRoomMsg(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nOmitSessionID )
{
	for ( auto& ref : m_vPlayers )
	{
		if ( ref && ref->getSessionID() != nOmitSessionID && ref->isOnline() )
		{
			sendMsgToPlayer(prealMsg, nMsgType, ref->getSessionID());
		}
	}

	// stand player 
	for (auto& ref : m_vStandPlayers)
	{
		auto p = ref.second;
		if ( p == nullptr )
		{
			continue;
		}

		if ( p->nSessionID != nOmitSessionID )
		{
			sendMsgToPlayer(prealMsg, nMsgType, p->nSessionID );
		}
	}
}

void GameRoom::sendMsgToPlayer(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nSessionID )
{
	getRoomMgr()->sendMsg(prealMsg, nMsgType, getRoomID(), nSessionID,ID_MSG_PORT_CLIENT );
}

bool GameRoom::onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)
{
	switch ( nMsgType )
	{
	case MSG_PLAYER_SIT_DOWN:
	{
		uint16_t nIdx = prealMsg["idx"].asUInt();
		stStandPlayer* pStand = nullptr;
		uint8_t nRet = 0;
		do
		{
			auto pPlayer = getPlayerBySessionID(nSessionID);
			if (pPlayer)
			{
				nRet = 4;
				break;
			}

			pPlayer = getPlayerByIdx(nIdx);
			if (pPlayer)
			{
				nRet = 1;
				break;
			}

			pStand = getStandPlayerBySessionID(nSessionID);
			if (!pStand)
			{
				nRet = 3;
				break;
			}

		} while (0);

		if (nRet)
		{
			Json::Value jsRet;
			jsRet["ret"] = nRet;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			break;
		}

		// go on  
		Json::Value jsReq;
		jsReq["targetUID"] = pStand->nUserUID;
		jsReq["roomID"] = getRoomID();
		jsReq["sessionID"] = nSessionID;
		auto pAsync = getRoomMgr()->getSvrApp()->getAsynReqQueue();
		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, pStand->nUserUID, eAsync_Request_EnterRoomInfo, jsReq, [nSessionID,this, nIdx](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut )
		{
			if (isTimeOut)
			{
				LOGFMTE("request time out uid = %u , already in other room id = %u , not in this room id = %u", retContent["uid"].asUInt(), 0, getRoomID());
				Json::Value jsRet;
				jsRet["ret"] = 2;
				sendMsgToPlayer(jsRet, MSG_PLAYER_SIT_DOWN, nSessionID);
				return;
			}

			auto nRet = retContent["ret"].asUInt();
			if ( 1 == nRet )
			{
			 
				Json::Value jsRet;
				jsRet["ret"] = 2;
				sendMsgToPlayer(jsRet, MSG_PLAYER_SIT_DOWN, nSessionID);
				return;
			}

			if (nRet)
			{
				Json::Value jsRet;
				jsRet["ret"] = 5;
				sendMsgToPlayer(jsRet, MSG_PLAYER_SIT_DOWN, nSessionID);
				return;
			}

			stEnterRoomData tInfo;
			tInfo.nUserUID = retContent["uid"].asUInt();
			tInfo.nSessionID = nSessionID;
			tInfo.nDiamond = retContent["diamond"].asUInt();
			tInfo.nChip = retContent["coin"].asUInt();
			doPlayerSitDown(&tInfo, nIdx);

			Json::Value jsRet;
			jsRet["ret"] = 0;
			sendMsgToPlayer(jsRet, MSG_PLAYER_SIT_DOWN, nSessionID);
		}, pStand->nUserUID);
	}
	break;
	case MSG_PLAYER_STAND_UP:
	{
		auto p = getPlayerBySessionID(nSessionID);
		Json::Value js;
		js["ret"] = 0;
		if (!p)
		{
			js["ret"] = 1;
			sendMsgToPlayer(js, nMsgType, nSessionID);
			break;
		}

		doPlayerStandUp(p->getUserUID());
		sendMsgToPlayer(js, nMsgType, nSessionID);
	}
	break;
	case MSG_PLAYER_LEAVE_ROOM:
	{
		uint16_t nUID = 0;
		auto pPlayer = getPlayerBySessionID(nSessionID);
		if (pPlayer == nullptr)
		{
			auto ps = getStandPlayerBySessionID(nSessionID);
			if (ps)
			{
				nUID = ps->nUserUID;
			}
		}
		else
		{
			nUID = pPlayer->getUserUID();
		}

		uint8_t nRet = 0;
		do
		{
			if ( 0 == nUID)
			{
				nRet = 1;
				break;
			}

			nRet = doPlayerLeaveRoom(nUID) ? 0 : 2;
		} while ( 0 );

		Json::Value js;
		js["ret"] = nRet;
		sendMsgToPlayer(js, nMsgType, nSessionID);
	}
	break;
	default:
		break; 
	}
	return true;
}

bool GameRoom::onPlayerNetStateRefreshed(uint32_t nPlayerID, eNetState nState)
{
	auto pPlayer = getPlayerByUID(nPlayerID);
	if (!pPlayer)
	{
		LOGFMTE( "inform player state refreshed , but player is null room id = %u , uid = %u",getRoomID(),nPlayerID );
		return false;
	}

	if (nState == eNetState::eNet_Offline)
	{
		doPlayerLeaveRoom(nPlayerID);
		return true;
	}

	pPlayer->setIsOnline(nState == eNetState::eNet_Online);
	// send msg update net state ;
	Json::Value jsNetState;
	jsNetState["idx"] = pPlayer->getIdx();
	jsNetState["uid"] = pPlayer->getUserUID();
	jsNetState["state"] = nState;
	sendRoomMsg(jsNetState, MSG_ROOM_REFRESH_NET_STATE);
	return true;
}

bool GameRoom::onPlayerSetNewSessionID(uint32_t nPlayerID, uint32_t nSessinID)
{
	auto pPlayer = getPlayerByUID(nPlayerID);
	if (!pPlayer)
	{
		LOGFMTE("inform player session id  refreshed , but player is null room id = %u , uid = %u", getRoomID(), nPlayerID);
		return false;
	}
	pPlayer->setNewSessionID(nSessinID);
	return true;
}

void GameRoom::sendRoomInfo(uint32_t nSessionID)
{
	Json::Value jsRoomInfo;
	jsRoomInfo["opts"] = m_jsOpts;
	packRoomInfo(jsRoomInfo);
	Json::Value jsArraPlayers;
	for (auto& ref : m_vPlayers)
	{
		if ( ref )
		{
			Json::Value jsPlayer;
			visitPlayerInfo(ref, jsPlayer);
			jsArraPlayers[jsArraPlayers.size()] = jsPlayer;
		}
	}
	jsRoomInfo["players"] = jsArraPlayers;
	sendMsgToPlayer(jsRoomInfo, MSG_ROOM_INFO, nSessionID);
}

void GameRoom::visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo)
{
	if (pPlayer == nullptr)
	{
		return;
	}

	jsPlayerInfo["idx"] = pPlayer->getIdx();
	jsPlayerInfo["uid"] = pPlayer->getUserUID();
	jsPlayerInfo["isOnline"] = pPlayer->isOnline() ? 1 : 0;
	jsPlayerInfo["chips"] = pPlayer->getChips();
}

IGameRoomManager* GameRoom::getRoomMgr()
{
	return m_pRoomMgr;
}

IGamePlayer* GameRoom::getPlayerByUID(uint32_t nUserUID)
{
	for (auto& ref : m_vPlayers)
	{
		if (ref && ref->getUserUID() == nUserUID )
		{
			return ref;
		}
	}
	return nullptr;
}

IGamePlayer* GameRoom::getPlayerBySessionID(uint32_t nSessionID)
{
	for (auto& ref : m_vPlayers)
	{
		if (ref && ref->getSessionID() == nSessionID )
		{
			return ref;
		}
	}
	return nullptr;
}

IGamePlayer* GameRoom::getPlayerByIdx(uint16_t nIdx)
{
	if (nIdx >= m_vPlayers.size() )
	{
		return nullptr;
	}
	return m_vPlayers[nIdx];
}

uint16_t GameRoom::getSeatCnt()
{
	return m_vPlayers.size();
}

IGameRoomDelegate* GameRoom::getDelegate()
{
	return m_pDelegate;
}

GameRoom::stStandPlayer* GameRoom::getStandPlayerBySessionID(uint32_t nSessinID)
{
	for (auto& ref : m_vStandPlayers)
	{
		if (ref.second->nSessionID == nSessinID)
		{
			return ref.second;
		}
	}
	return nullptr;
}

GameRoom::stStandPlayer* GameRoom::getStandPlayerByUID(uint32_t nUserID)
{
	auto iter = m_vStandPlayers.find(nUserID);
	if (iter == m_vStandPlayers.end())
	{
		return nullptr;
	}
	return iter->second;
}