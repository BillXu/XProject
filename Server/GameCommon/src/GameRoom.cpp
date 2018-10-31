#include "GameRoom.h"
#include "IGameRoomManager.h"
#include "IGamePlayer.h"
#include "IGameRoomDelegate.h"
#include "log4z.h"
#include "ISeverApp.h"
#include "AsyncRequestQuene.h"
#include <ctime>
#include "IGameRoomState.h"
#include "IPoker.h"
bool GameRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)
{
	m_nTempID = 0;
	m_pDelegate = nullptr;
	m_pRoomMgr = pRoomMgr;
	m_nSieralNum = nSeialNum;
	m_nRoomID = nRoomID;
	m_vPlayers.resize(nSeatCnt);
	m_jsOpts = vJsOpts;
	m_vStandPlayers.clear();
	getPoker()->init(vJsOpts);

	m_ptrCurRoundRecorder = nullptr;
	m_ptrRoomRecorder = createRoomRecorder();
	m_ptrRoomRecorder->init(nSeialNum, nRoomID, getRoomType(), vJsOpts["uid"].asUInt(),vJsOpts["clubID"].asUInt() ,vJsOpts);

	m_ptrGameReplay = std::make_shared<MJReplayGame>();
	m_ptrGameReplay->init(m_pRoomMgr->getSvrApp()->getLocalSvrMsgPortType(), vJsOpts);
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

	for (auto& ref : m_vAllState )
	{
		if (ref.second)
		{
			delete ref.second;
			ref.second = nullptr;
		}
	}
	m_vAllState.clear();
}

void GameRoom::setDelegate(IGameRoomDelegate* pDelegate)
{
	m_pDelegate = pDelegate;
}

uint8_t GameRoom::checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)
{
	if ( getPlayerByUID(pEnterRoomPlayer->nUserUID) )
	{
		return 0;
	}

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
		LOGFMTE("this player is already sitdown uid = %u , room id = %u , why enter room again ?",psitDown->getUserUID(),getRoomID() );
		//sendRoomInfo(pEnterRoomPlayer->nSessionID);
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
	//sendRoomInfo(pEnterRoomPlayer->nSessionID);
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
	// save room recorder 
	getRoomRecorder()->doSaveRoomRecorder( getRoomMgr()->getSvrApp()->getAsynReqQueue() );

	// process player leave ;
	std::vector<uint32_t> vAllInRoomPlayers;
	for (auto& pPayer : m_vPlayers)
	{
		if ( pPayer == nullptr )
		{
			continue;
		}
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
	m_bSaveRecorder = false;

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

	getPoker()->shuffle();
	// game replayer 
	m_ptrGameReplay->startNewReplay(getRoomMgr()->generateReplayID());
	// cur round recorder ;
	m_ptrCurRoundRecorder = createSingleRoundRecorder();
	m_ptrCurRoundRecorder->init(getRoomRecorder()->getRoundRecorderCnt(), time(nullptr), m_ptrGameReplay->getReplayID() );
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

void GameRoom::saveGameRecorder() {
	if (m_bSaveRecorder) {
		return;
	}

	m_bSaveRecorder = true;

	// cacualte player offset ;
	if (getDelegate() && getDelegate()->isEnableRecorder())
	{
		for (auto& ref : m_vPlayers)
		{
			if (!ref || ref->haveState(eRoomPeer_StayThisRound) == false)
			{
				continue;
			}

			auto pPlayerRecorder = createPlayerRecorderPtr();
			if (false == ref->recorderVisitor(pPlayerRecorder))
			{
				LOGFMTE("player recorder visitor error can not save , room id = %u, uid = %u", getRoomID(), ref->getUserUID());
				continue;
			}
			getCurRoundRecorder()->addPlayerRecorderInfo(pPlayerRecorder);
		}
		getRoomRecorder()->addSingleRoundRecorder(getCurRoundRecorder());
	}

	// do save this game replay
	if (getDelegate() && getDelegate()->isEnableReplay())
	{
		m_ptrGameReplay->doSaveReplayToDB(getRoomMgr()->getSvrApp()->getAsynReqQueue());
	}
}

void GameRoom::onGameEnd()
{
	saveGameRecorder();
	//// cacualte player offset ;
	//if (getDelegate() && getDelegate()->isEnableRecorder() )
	//{
	//	for (auto& ref : m_vPlayers)
	//	{
	//		if ( !ref  || ref->haveState(eRoomPeer_StayThisRound) == false )
	//		{
	//			continue;
	//		}

	//		auto pPlayerRecorder = createPlayerRecorderPtr();
	//		if (false == ref->recorderVisitor(pPlayerRecorder))
	//		{
	//			LOGFMTE( "player recorder visitor error can not save , room id = %u, uid = %u",getRoomID(),ref->getUserUID());
	//			continue;
	//		}
	//		getCurRoundRecorder()->addPlayerRecorderInfo(pPlayerRecorder);
	//	}
	//	getRoomRecorder()->addSingleRoundRecorder(getCurRoundRecorder());
	//}

	//// do save this game replay
	//if (getDelegate() && getDelegate()->isEnableReplay())
	//{
	//	m_ptrGameReplay->doSaveReplayToDB(getRoomMgr()->getSvrApp()->getAsynReqQueue());
	//}
	
	if (getDelegate())
	{
		getDelegate()->onGameEnd(this);
	}

	// all player game end ;
	for (auto& ref : m_vPlayers)
	{
		if (ref)
		{
			ref->onGameEnd();
		}
	}
}

void GameRoom::visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo,uint32_t nVisitorSessionID)
{
	if ( pPlayer == nullptr )
	{
		return;
	}
	//{ idx: 23, uid : 23, isOnline : 0, chips : 23 ...}
	jsPlayerInfo["idx"] = pPlayer->getIdx();
	jsPlayerInfo["uid"] = pPlayer->getUserUID();
	jsPlayerInfo["isOnline"] = pPlayer->isOnline() ? 1 : 0;
	jsPlayerInfo["chips"] = pPlayer->getChips();
	jsPlayerInfo["state"] = pPlayer->getState();
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
	visitPlayerInfo(p, jsRoomPlayerSitDown,0 );
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
		LOGFMTE( "room id = %u palyer uid = %u£¬not sit down , how to stand up ?",getRoomID(),nUserUID );
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
	jsReqLeave["port"] = getRoomMgr()->getSvrApp()->getLocalSvrMsgPortType();
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
	m_pCurState->update(fDelta);
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

void GameRoom::sendRoomMsgToStander(Json::Value& prealMsg, uint16_t nMsgType) {
	// stand player 
	for (auto& ref : m_vStandPlayers)
	{
		auto p = ref.second;
		if (p == nullptr)
		{
			continue;
		}

		sendMsgToPlayer(prealMsg, nMsgType, p->nSessionID);
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

			pStand = getStandPlayerBySessionID(nSessionID);
			if (!pStand)
			{
				nRet = 3;
				break;
			}

			if ( nIdx >= getSeatCnt() ) // find empty pos
			{
				auto nCheckIdx = rand() % getSeatCnt();
				for ( ; nCheckIdx < getSeatCnt() * 2; ++nCheckIdx)
				{
					nIdx = nCheckIdx % getSeatCnt();
					auto p = getPlayerByIdx(nIdx);
					if ( !p )
					{
						break;
					}
				}
			}

			pPlayer = getPlayerByIdx(nIdx);
			if (pPlayer || nIdx >= getSeatCnt())
			{
				nRet = 1;
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
		jsReq["port"] = getRoomMgr()->getSvrApp()->getLocalSvrMsgPortType();
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

			auto pPlayer = getPlayerByIdx(nIdx);
			if (pPlayer)
			{
				Json::Value jsRet;
				jsRet["ret"] = 1;
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
		uint32_t nUID = 0;
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
	case MSG_ROOM_KICK_PLAYER:
	{
		Json::Value js;
		uint32_t nTargetUID = prealMsg["targetUID"].asUInt();
		auto targetPlayer = getPlayerByUID(nTargetUID);
		uint32_t nTargetSessionID = 0;
		if (targetPlayer) {
			nTargetSessionID = targetPlayer->getSessionID();
		}
		uint8_t nRet = doPlayerLeaveRoom(nTargetUID) ? 0 : 2;
		js = prealMsg;
		js["ret"] = nRet;
		js["roomID"] = getRoomID();
		sendMsgToPlayer(js, nMsgType, nSessionID);
		if (nRet == 0 && nTargetSessionID) {
			sendMsgToPlayer(js, nMsgType, nTargetSessionID);
		}
		LOGFMTE("do player uid = %u kick player uid = %u, ret = %u", prealMsg["uid"].asUInt(), nTargetUID, nRet);
	}
	break;
	case MSG_PLAYER_INTERACT_EMOJI:
	{
		auto pPlayer = getPlayerBySessionID(nSessionID);
		if (!pPlayer)
		{
			Json::Value js;
			js["ret"] = 3;
			sendMsgToPlayer(js, nMsgType, nSessionID);
			break;
		}

		// go on  
		Json::Value jsReq;
		jsReq["targetUID"] = pPlayer->getUserUID();
		jsReq["cnt"] = 1;
		jsReq["roomID"] = getRoomID();
		auto pAsync = getRoomMgr()->getSvrApp()->getAsynReqQueue();
		prealMsg["invokerIdx"] = pPlayer->getIdx();
		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, pPlayer->getUserUID(), eAsync_Comsume_Interact_Emoji, jsReq, [nSessionID, this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
		{
			if (isTimeOut)
			{
				Json::Value js;
				js["ret"] = 2;
				sendMsgToPlayer(js, MSG_PLAYER_INTERACT_EMOJI, nSessionID);
				LOGFMTE("wait time out can not send emoj room id = %u , sessionid = %u",getRoomID(),nSessionID );
				return;
			}

			auto nRet = retContent["ret"].asUInt();
			if ( 0 != nRet)
			{
				Json::Value js;
				js["ret"] = nRet;
				sendMsgToPlayer(js, MSG_PLAYER_INTERACT_EMOJI, nSessionID);
				return;
			}

			sendRoomMsg(jsUserData, MSG_ROOM_INTERACT_EMOJI );
		}, prealMsg,pPlayer->getUserUID());
	}
	break;
	default:
		return getCurState()->onMsg(prealMsg,nMsgType,eSenderPort,nSessionID);
	}
	return true;
}

bool GameRoom::onPlayerNetStateRefreshed(uint32_t nPlayerID, eNetState nState)
{
	auto pPlayer = getPlayerByUID(nPlayerID);
	if (!pPlayer)
	{
		auto pStand = getStandPlayerByUID(nPlayerID);
		if ( pStand )
		{
			if (nState == eNetState::eNet_Offline)
			{
				doPlayerLeaveRoom(nPlayerID);
				return true;
			}

			return true;
		}
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

	if ( pPlayer->isOnline() == false )
	{
		// send msg update net state ;
		Json::Value jsNetState;
		jsNetState["idx"] = pPlayer->getIdx();
		jsNetState["uid"] = pPlayer->getUserUID();
		jsNetState["state"] = eNetState::eNet_Online;
		sendRoomMsg(jsNetState, MSG_ROOM_REFRESH_NET_STATE);
	}

	pPlayer->setNewSessionID(nSessinID);
	return true;
}

void GameRoom::packRoomInfo(Json::Value& jsRoomInfo)
{
	jsRoomInfo["opts"] = m_jsOpts;
	jsRoomInfo["roomID"] = getRoomID();

	Json::Value jsStateInfo;
	getCurState()->roomInfoVisitor(jsStateInfo);
	jsRoomInfo["stateInfo"] = jsStateInfo;
	jsRoomInfo["state"] = getCurState()->getStateID();
	jsRoomInfo["stateTime"] = uint8_t(getCurState()->getStateDuring() + 0.8);
	jsRoomInfo["stateWaitTime"] = getCurState()->getWaitTime();
}

void GameRoom::sendRoomPlayersInfo(uint32_t nSessionID)
{
	Json::Value jsArraPlayers;
	for (auto& ref : m_vPlayers)
	{
		if (ref)
		{
			Json::Value jsPlayer;
			visitPlayerInfo(ref, jsPlayer, nSessionID);
			jsArraPlayers[jsArraPlayers.size()] = jsPlayer;
		}
	}

	/*if ( jsArraPlayers.size() == 0 )
	{
		return;
	}*/
	Json::Value jsPlayersInfo;
	jsPlayersInfo["players"] = jsArraPlayers;
	LOGFMTI( "send playes info" );
	sendMsgToPlayer(jsPlayersInfo, MSG_ROOM_PLAYER_INFO, nSessionID);
}

void GameRoom::sendRoomInfo(uint32_t nSessionID)
{
	Json::Value jsRoomInfo;
	packRoomInfo(jsRoomInfo);
	LOGFMTI("send room info game room");
	sendMsgToPlayer(jsRoomInfo, MSG_ROOM_INFO, nSessionID);

	// send players 
	sendRoomPlayersInfo(nSessionID);
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

std::shared_ptr<IGameRoomRecorder> GameRoom::getRoomRecorder()
{
	return m_ptrRoomRecorder;
}

std::shared_ptr<ISingleRoundRecorder> GameRoom::getCurRoundRecorder()
{
	return m_ptrCurRoundRecorder;
}

std::shared_ptr<IGameRoomRecorder> GameRoom::createRoomRecorder()
{
	auto ptr = std::make_shared<IGameRoomRecorder>();
	return ptr;
}

std::shared_ptr<ISingleRoundRecorder> GameRoom::createSingleRoundRecorder()
{
	return std::make_shared<ISingleRoundRecorder>();
}

IGameRoomState* GameRoom::getCurState()
{
	return m_pCurState;
}

void GameRoom::goToState(IGameRoomState* pTargetState, Json::Value* jsValue )
{
	if ( !pTargetState )
	{
		LOGFMTE( "target state is null room id = %u",getRoomID() );
		return;
	}
	
	if (nullptr == m_pCurState)
	{
		LOGFMTE( "why cur state is null ? room id = %u",getRoomID() );
		return;
	}

	Json::Value jsMsg;
	jsMsg["lastState"] = m_pCurState->getStateID();

	m_pCurState->leaveState();
	m_pCurState = pTargetState;
	if (jsValue == nullptr)
	{
		Json::Value js;
		m_pCurState->enterState(this, js);
	}
	else
	{
		m_pCurState->enterState(this, *jsValue);
	}

	jsMsg["newState"] = m_pCurState->getStateID();
	jsMsg["stateTime"] = m_pCurState->getStateDuring();
	sendRoomMsg(jsMsg, MSG_ROOM_CHANGE_STATE );
}

void GameRoom::goToState(uint32_t nStateID, Json::Value* jsValue)
{
	auto iter = m_vAllState.find(nStateID);
	if (iter != m_vAllState.end())
	{
		return goToState(iter->second,jsValue);
	}
	LOGFMTE( "room id = %u go to state , targetstate id = %u is null",getRoomID(),nStateID );
	return;
}

void GameRoom::setInitState(IGameRoomState* pTargetState)
{
	m_pCurState = pTargetState;
	Json::Value jsNull;
	m_pCurState->enterState(this, jsNull);
}

bool GameRoom::addRoomState(IGameRoomState* pTargetState)
{
	auto iter = m_vAllState.find(pTargetState->getStateID());
	if ( iter != m_vAllState.end() )
	{
		LOGFMTE("already add state id = %u , do not add again ",pTargetState->getStateID() );
		return false;
	}
	m_vAllState[pTargetState->getStateID()] = pTargetState;
	return true;
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

uint16_t GameRoom::getPlayerCnt()
{
	uint16_t nCnt = 0;
	auto nSeatCnt = getSeatCnt();
	for (auto nPlayerIdx = 0; nPlayerIdx < nSeatCnt; ++nPlayerIdx)
	{
		auto p = getPlayerByIdx(nPlayerIdx);
		if (p)
		{
			++nCnt;
		}
	}
	return nCnt;
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

std::shared_ptr<IPlayerRecorder> GameRoom::createPlayerRecorderPtr()
{
	return std::make_shared<IPlayerRecorder>();
}

bool GameRoom::addReplayFrame(uint32_t nFrameType, Json::Value& jsFrameArg)
{
	if ((nullptr == getDelegate()) || (false == getDelegate()->isEnableReplay()))
	{
		return false;
	}

	m_ptrGameReplay->addFrame(nFrameType, jsFrameArg);
	return true;
}

bool GameRoom::checkPlayerInThisRoom(uint32_t nSessionID) {
	if (getPlayerBySessionID(nSessionID)) {
		return true;
	}

	if (getStandPlayerBySessionID(nSessionID)) {
		return true;
	}

	return false;
}