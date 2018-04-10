#include "ThirteenPrivateRoom.h"
#include "Thirteen\ThirteenRoom.h"
#include "IGamePlayer.h"
#include "IGameRoomState.h"
#include "IGameRoomManager.h"
#include "AsyncRequestQuene.h"
#include "../ServerCommon/ISeverApp.h"
#define OFFLINE_AUTO_LEAVE_ROOM 2
ThirteenPrivateRoom::~ThirteenPrivateRoom() {
	/*for (auto& ref : m_vPRooms) {
		if (ref) {
			if (m_pRoom == ref) {
				m_pRoom = nullptr;
			}
			delete ref;
			ref = nullptr;
		}
	}
	m_vPRooms.clear();*/
	for (auto ref : m_mStayPlayers) {
		if (ref.second) {
			delete ref.second;
			ref.second = nullptr;
		}
	}
	m_mStayPlayers.clear();
}

bool ThirteenPrivateRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)
{
	if (vJsOpts["overType"].isNull() || vJsOpts["overType"].isUInt() == false) {
		m_nOverType = ROOM_OVER_TYPE_TIME;
	}
	else {
		m_nOverType = vJsOpts["overType"].asUInt();
	}
	if (m_nOverType >= ROOM_OVER_TYPE_MAX) {
		m_nOverType = ROOM_OVER_TYPE_TIME;
	}

	if (IPrivateRoom::init(pRoomMgr, nSeialNum, nRoomID, nSeatCnt, vJsOpts)) {
		/*if (m_pRoom) {
			m_vPRooms.push_back(m_pRoom);
		}*/

		m_isForbitEnterRoomWhenStarted = vJsOpts["forbidJoin"].asUInt() == 1;
		m_nAutoOpenCnt = vJsOpts["starGame"].asUInt();
		//m_nMaxCnt = vJsOpts["maxSeatCnt"].asUInt();
		/*if (m_nMaxCnt < nSeatCnt) {
			m_nMaxCnt = nSeatCnt;
		}*/

		if (vJsOpts["clubID"].isNull() || vJsOpts["clubID"].isUInt() == false) {
			m_nClubID = 0;
		}
		else {
			m_nClubID = vJsOpts["clubID"].asUInt();
		}

		if (vJsOpts["leagueID"].isNull() || vJsOpts["leagueID"].isUInt() == false) {
			m_nLeagueID = 0;
		}
		else {
			m_nLeagueID = vJsOpts["leagueID"].asUInt();
		}

		if (m_nOverType == ROOM_OVER_TYPE_TIME) {
			// start auto dismiss timer ;
			m_tCreateTimeLimit.reset();
			m_tCreateTimeLimit.setInterval(m_nLeftRounds * 60);
#ifdef _DEBUG
			//m_tCreateTimeLimit.setInterval(60 * 5);
#endif // _DEBUG

			m_tCreateTimeLimit.setIsAutoRepeat(false);
			m_tCreateTimeLimit.setCallBack([this](CTimer*p, float f) {
				/*m_tInvokerTime = 0;
				m_nApplyDismissUID = 0;
				LOGFMTI("system auto dismiss room id = %u , owner id = %u", getRoomID(), m_nOwnerUID);
				doRoomGameOver(true);*/
				m_nLeftRounds = 0;
			});
			m_tCreateTimeLimit.start();
			//m_tCreateTimeLimit.getDuringTime();
		}

		m_ptrRoomRecorder = getCoreRoom()->createRoomRecorder();
		m_ptrRoomRecorder->init(nSeialNum, nRoomID, getCoreRoom()->getRoomType(), vJsOpts["uid"].asUInt(), vJsOpts);
		m_ptrRoomRecorder->setClubID(m_nClubID);
		m_ptrRoomRecorder->setLeagueID(m_nLeagueID);
		m_nRotBankerPool = 0;
		return true;
	}
	
	return false;
}

void ThirteenPrivateRoom::packRoomInfo(Json::Value& jsRoomInfo) {
	IPrivateRoom::packRoomInfo(jsRoomInfo);
	if (m_nOverType == ROOM_OVER_TYPE_TIME) {
		jsRoomInfo["leftTime"] = (int32_t)m_tCreateTimeLimit.getDuringTime();
		jsRoomInfo["time"] = (int32_t)m_tCreateTimeLimit.getInterval();
	}
	jsRoomInfo["RBPool"] = m_nRotBankerPool;
}

uint8_t ThirteenPrivateRoom::checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)
{
	if (m_isForbitEnterRoomWhenStarted && isRoomStarted() )
	{
		return 7;
	}

	auto sPlayer = isEnterByUserID(pEnterRoomPlayer->nUserUID);
	if (sPlayer && sPlayer->isTOut) {
		return 11;
	}

	return IPrivateRoom::checkPlayerCanEnter(pEnterRoomPlayer);
}

GameRoom* ThirteenPrivateRoom::doCreatRealRoom()
{
	return new ThirteenRoom();
}

uint8_t ThirteenPrivateRoom::getInitRound( uint8_t nLevel )
{
	//uint8_t nAmountLevel = nLevel >> 4;
	uint8_t nTypeLevel = nLevel << 4;
	nTypeLevel = nTypeLevel >> 4;
	switch (m_nOverType) {
	case ROOM_OVER_TYPE_TIME:
	{
		uint8_t vJun[] = {10, 15, 20, 30, 60};
		if (nTypeLevel >= sizeof(vJun) / sizeof(uint8_t))
		{
			LOGFMTE("invalid level type = %u", nLevel);
			nTypeLevel = 0;
		}
#ifdef _DEBUG
		if (nTypeLevel == 4) {
			return 2;
		}
#endif // _DEBUG

		return vJun[nTypeLevel];
	}
	case ROOM_OVER_TYPE_ROUND:
	{
		uint8_t vJun[] = { 10 , 20 , 30 };
		if (nTypeLevel >= sizeof(vJun) / sizeof(uint8_t))
		{
			LOGFMTE("invalid level type = %u", nLevel);
			nTypeLevel = 0;
		}
		return vJun[nTypeLevel];
	}
	default:
	{
		m_nOverType = ROOM_OVER_TYPE_TIME;
		uint8_t vJun[] = { 10, 15, 20, 30, 60 };
		if (nTypeLevel >= sizeof(vJun) / sizeof(uint8_t))
		{
			LOGFMTE("invalid level type = %u", nLevel);
			nTypeLevel = 0;
		}
		return vJun[nTypeLevel];
	}
	}

	/*uint8_t vJun[] = { 10 , 20 , 30 };
	if ( nLevel >= sizeof(vJun) / sizeof(uint8_t) )
	{
		LOGFMTE( "invalid level type = %u",nLevel );
		nLevel = 0;
	}
	return vJun[nLevel];*/
}

void ThirteenPrivateRoom::doSendRoomGameOverInfoToClient( bool isDismissed )
{
	// send room over msg ;
	//Json::Value jsArrayPlayers;
	/*auto nCnt = getCoreRoom()->getSeatCnt();
	for ( uint16_t nIdx = 0; nIdx < nCnt; ++nIdx)
	{
		auto pPlayer = getCoreRoom()->getPlayerByIdx(nIdx);
		if (pPlayer == nullptr)
		{
			continue;
		}
		Json::Value jsPlayer;
		jsPlayer["uid"] = pPlayer->getUserUID();
		jsPlayer["final"] = pPlayer->getChips();
		jsArrayPlayers[jsArrayPlayers.size()] = jsPlayer;
	}*/
	/*for (auto ref : m_mStayPlayers) {
		if (ref.second->isSitdown) {
			Json::Value jsPlayer;
			jsPlayer["uid"] = ref.second->nUserUID;
			jsPlayer["final"] = ref.second->nChip;
			jsArrayPlayers[jsArrayPlayers.size()] = jsPlayer;
		}
	}*/

	Json::Value jsMsg;
	jsMsg["dismissID"] = m_nApplyDismissUID;
	//jsMsg["result"] = jsArrayPlayers;
	jsMsg["sieralNum"] = getSeiralNum();
	jsMsg["joinAmount"] = getPlayerCnt();
	sendRoomMsg(jsMsg, MSG_ROOM_GAME_OVER );
}

bool ThirteenPrivateRoom::canStartGame(IGameRoom* pRoom)
{
	// check room over
	if (0 == m_nLeftRounds)
	{
		doRoomGameOver(false);
		return false;
	}

	uint8_t nCnt = 0;
	auto pThirteenRoom = ((ThirteenRoom*)pRoom);
	for (uint8_t nIdx = 0; nIdx < pThirteenRoom->getSeatCnt(); ++nIdx)
	{
		auto pPlayer = pThirteenRoom->getPlayerByIdx(nIdx);
		if (pPlayer == nullptr) {
			continue;
		}
		if (pPlayer->haveState(eRoomPeer_Ready) == false && pPlayer->haveState(eRoomPeer_WaitDragIn) == false)
		{
			return false;
			//continue;
		}
		if (pPlayer->haveState(eRoomPeer_Ready))
		{
			++nCnt;
		}
	}

	bool canStart = false;

	if ( m_isOpen/* && m_nAutoOpenCnt > 0*/)
	{	
		canStart = nCnt > 1;
	}
	else {
		canStart = nCnt >= m_nAutoOpenCnt;
	}

	if (canStart)
	{
		m_isOpen = true;
		Json::Value js;
		pRoom->sendRoomMsg(js, MSG_ROOM_DO_OPEN);
		LOGFMTI(" room id = %u auto set open", getRoomID());
	}

	return canStart;
}

void ThirteenPrivateRoom::onPlayerWillStandUp(IGameRoom* pRoom, IGamePlayer* pPlayer) {
	if (m_mStayPlayers.count(pPlayer->getUserUID())) {
		m_mStayPlayers[pPlayer->getUserUID()]->nChip = pPlayer->getChips();
	}
}

void ThirteenPrivateRoom::onGameDidEnd(IGameRoom* pRoom) {
	// decrease round 
	if (m_nOverType == ROOM_OVER_TYPE_ROUND) {
		if (m_nLeftRounds > 0) {
			--m_nLeftRounds;
		}
	}

	// consume diamond 
	if (m_isOneRoundNormalEnd == false)
	{
		m_isOneRoundNormalEnd = true;
		auto nNeedDiamond = getDiamondNeed(m_nRoundLevel, getPayType());
		if (isAAPay() && nNeedDiamond > 0)  // only aa delay consum diamond , owner pay diamond mode , diamond was consumed when create the room ;
		{
			auto nCnt = m_pRoom->getSeatCnt();
			for (uint8_t nIdx = 0; nIdx < nCnt; ++nIdx)
			{
				auto pPlayer = m_pRoom->getPlayerByIdx(nIdx);
				if (!pPlayer)
				{
					//LOGFMTE( "player is null , comuse diamond idx = %u , room id = %u",nIdx , getRoomID() );
					continue;
				}

				Json::Value js;
				js["playerUID"] = pPlayer->getUserUID();
				js["diamond"] = nNeedDiamond;
				js["roomID"] = getRoomID();
				js["reason"] = 0;
				auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
				pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, pPlayer->getUserUID(), eAsync_Consume_Diamond, js);
			}
		}
	}

	auto nCnt = m_pRoom->getSeatCnt();
	for (uint8_t nIdx = 0; nIdx < nCnt; ++nIdx) {
		auto pPlayer = m_pRoom->getPlayerByIdx(nIdx);
		if (!pPlayer || pPlayer->haveState(eRoomPeer_StayThisRound) == false)
		{
			//LOGFMTE( "player is null , comuse diamond idx = %u , room id = %u",nIdx , getRoomID() );
			continue;
		}
		auto stg = isEnterByUserID(pPlayer->getUserUID());
		if (stg) {
			stg->isJoin += 1;
			if (stg->nState == eNet_Offline || stg->nState == eNet_WaitReconnect) {
				stg->nOffLineGame += 1;
				if (stg->nOffLineGame >= OFFLINE_AUTO_LEAVE_ROOM) {
					if (stg->nState == eNet_Offline) {
						getCoreRoom()->doPlayerLeaveRoom(stg->nUserUID);
					}
					else {
						getCoreRoom()->doPlayerStandUp(stg->nUserUID);
					}
					stg->nOffLineGame = 0;
				}
			}
			else {
				stg->nOffLineGame = 0;
			}
		}
	}

	// check room over
	if (0 == m_nLeftRounds)
	{
		doRoomGameOver(false);
	}
}

void ThirteenPrivateRoom::onPlayerWaitDragIn(uint32_t nUserUID) {
	//getCoreRoom()->getPlayerByUID();
}

void ThirteenPrivateRoom::onPlayerApplyDragIn(uint32_t nUserUID, uint32_t nClubID) {
	if (m_mStayPlayers.count(nUserUID)) {
		m_mStayPlayers[nUserUID]->nClubID = nClubID;
	}
}

bool ThirteenPrivateRoom::onPlayerDeclineDragIn(uint32_t nUserID) {
	if (m_mStayPlayers.count(nUserID) == 0) {
		return false;
	}

	if (((ThirteenRoom*)getCoreRoom())->onPlayerDeclineDragIn(nUserID)) {
		
	}

	auto st = m_mStayPlayers[nUserID];
	if (st->isDragIn == false) {
		st->nClubID = 0;
	}
	return true;
}

bool ThirteenPrivateRoom::onPlayerDragIn(uint32_t nUserID, uint32_t nClubID, uint32_t nAmount) {
	if (nAmount == 0) {
		return false;
	}
	if (m_mStayPlayers.count(nUserID) == 0) {
		return false;
	}
	auto st = m_mStayPlayers[nUserID];
	if (st->nClubID && st->nClubID != nClubID) {
		return false;
	}
	if (((ThirteenRoom*)getCoreRoom())->onPlayerDragIn(nUserID, nAmount)) {
		st->nChip = getCoreRoom()->getPlayerByUID(nUserID)->getChips();
	}
	else {
		st->nChip += nAmount;
	}
	st->isDragIn = true;
	st->nAllWrag += nAmount;
	st->nClubID = nClubID;
	getRoomRecorder()->addDragIn(nUserID, nAmount, nClubID);
	return true;
}

bool ThirteenPrivateRoom::doDeleteRoom() {
	m_tWaitReplyDismissTimer.canncel();
	m_tAutoDismissTimer.canncel();
	// tell client closed room ;
	/*Json::Value jsDoClosed;
	jsDoClosed["roomID"] = getRoomID();
	jsDoClosed["isDismiss"] = m_nApplyDismissUID > 0 ? 1 : 0;
	sendRoomMsg(jsDoClosed, MSG_VIP_ROOM_DO_CLOSED);*/

	// tell data svr , the room is closed 
	Json::Value jsReqInfo;
	jsReqInfo["targetUID"] = m_nOwnerUID;
	jsReqInfo["roomID"] = getRoomID();
	jsReqInfo["port"] = m_pRoomMgr->getSvrApp()->getLocalSvrMsgPortType();
	auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
	pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, m_nOwnerUID, eAsync_Inform_RoomDeleted, jsReqInfo);

	for (auto& ref : m_mStayPlayers) {
		jsReqInfo["targetUID"] = ref.second->nUserUID;
		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, ref.second->nUserUID, eAsync_player_DragInRoom_Closed, jsReqInfo);
	}

	//auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
	getRoomRecorder()->doSaveRoomRecorder(pAsync);
	if (m_nLeagueID) {
		Json::Value jsReqInfo;
		jsReqInfo["leagueID"] = m_nLeagueID;
		jsReqInfo["roomID"] = getRoomID();
		//auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, m_nLeagueID, eAsync_league_or_club_DeleteRoom, jsReqInfo);
	}
	else if (m_nClubID) {
		Json::Value jsReqInfo;
		jsReqInfo["clubID"] = m_nClubID;
		jsReqInfo["roomID"] = getRoomID();
		//auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, m_nClubID, eAsync_league_or_club_DeleteRoom, jsReqInfo);
	}

	return m_pRoom->doDeleteRoom();

	//if (m_pRoom->doDeleteRoom()) {
	//	auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
	//	getRoomRecorder()->doSaveRoomRecorder(pAsync);
	//	if (m_nLeagueID) {
	//		Json::Value jsReqInfo;
	//		jsReqInfo["leagueID"] = m_nLeagueID;
	//		jsReqInfo["roomID"] = getRoomID();
	//		//auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
	//		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, m_nLeagueID, eAsync_league_or_club_DeleteRoom, jsReqInfo);
	//	}
	//	else if (m_nClubID) {
	//		Json::Value jsReqInfo;
	//		jsReqInfo["clubID"] = m_nClubID;
	//		jsReqInfo["roomID"] = getRoomID();
	//		//auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
	//		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, m_nClubID, eAsync_league_or_club_DeleteRoom, jsReqInfo);
	//	}
	//	return true;
	//}
	//return false;
}

bool ThirteenPrivateRoom::isRoomGameOver() {
	/*switch (m_nOverType) {
	case ROOM_OVER_TYPE_TIME:
	{
		return m_nLeftRounds < 1;
	}
	case ROOM_OVER_TYPE_ROUND:
	{
		return m_nLeftRounds <= 1;
	}
	}*/
	//return true;
	return m_nLeftRounds < 1;
}

std::shared_ptr<IGameRoomRecorder> ThirteenPrivateRoom::getRoomRecorder() {
	return m_ptrRoomRecorder;
}

uint32_t ThirteenPrivateRoom::getRoomPlayerCnt() {
	uint32_t nCnt = 0;
	if (((ThirteenRoom*)getCoreRoom())->isClubRoom()) {
		for (auto ref : m_mStayPlayers) {
			if (ref.second->nChip) {
				nCnt++;
			}
		}
	}
	else {
		nCnt = m_mStayPlayers.size();
	}
	
	return nCnt;
}

uint32_t ThirteenPrivateRoom::getClubID() {
	auto pRoom = (ThirteenRoom*)getCoreRoom();
	if (pRoom) {
		return pRoom->isClubRoom();
	}
	return 0;
}

uint32_t ThirteenPrivateRoom::getLeagueID() {
	auto pRoom = (ThirteenRoom*)getCoreRoom();
	if (pRoom) {
		return pRoom->isLeagueRoom();
	}
	return 0;
}

uint32_t ThirteenPrivateRoom::getDragInClubID(uint32_t nUserID) {
	if (isEnterByUserID(nUserID)) {
		return isEnterByUserID(nUserID)->nClubID;
	}
	return 0;
}

uint32_t ThirteenPrivateRoom::getEnterClubID(uint32_t nUserID) {
	if (isEnterByUserID(nUserID)) {
		return isEnterByUserID(nUserID)->nEnterClubID;
	}
	return 0;
}

uint16_t ThirteenPrivateRoom::getPlayerCnt() {
	uint16_t nCnt = 0;
	for (auto ref : m_mStayPlayers) {
		if (ref.second->isSitdown) {
			nCnt++;
		}
	}
	return nCnt;
}

bool ThirteenPrivateRoom::onPlayerSetNewSessionID(uint32_t nPlayerID, uint32_t nSessinID) {
	auto st = isEnterByUserID(nPlayerID);
	if (st) {
		st->nSessionID = nSessinID;
		st->nState = eNet_Online;
	}
	return IPrivateRoom::onPlayerSetNewSessionID(nPlayerID, nSessinID);
}

void ThirteenPrivateRoom::doRoomGameOver(bool isDismissed) {
	getRoomRecorder()->setPlayerCnt(getRoomPlayerCnt());
	getRoomRecorder()->setRotBankerPool(m_nRotBankerPool);
	getRoomRecorder()->setDuration(m_tCreateTimeLimit.getInterval());

	auto pRoom = (ThirteenRoom*)getCoreRoom();
	pRoom->doAllPlayerStandUp();
	auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
	for (auto ref : m_mStayPlayers) {
		auto nClubID = ref.second->nClubID;
		if (nClubID) {
			Json::Value jsReqInfo;
			jsReqInfo["targetUID"] = ref.second->nUserUID;
			jsReqInfo["chip"] = ref.second->nChip;
			jsReqInfo["roomID"] = getRoomID();
			jsReqInfo["clubID"] = nClubID;
			pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, ref.second->nUserUID, eAsync_player_clubRoom_Back_Chip, jsReqInfo);

			auto nLeagueID = getLeagueID();
			if (nLeagueID) {
				jsReqInfo["uid"] = ref.second->nUserUID;
				jsReqInfo["leagueID"] = nLeagueID;
				pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, ref.second->nUserUID, eAsync_league_clubRoom_Back_Integration, jsReqInfo);
			}
		}
	}

	if (eState_RoomOvered == m_nPrivateRoomState)  // avoid opotion  loop invoke this function ;
	{
		LOGFMTE("already gave over , why invoker again room id = %u", getRoomID());
		return;
	}
	// do close room ;
	//if (isRoomStarted())
	//{
	//	// prepare game over bills 
	//	doSendRoomGameOverInfoToClient(isDismissed);
	//}
	doSendRoomGameOverInfoToClient(isDismissed);

	// give back room card to room owner ;
	bool isAlreadyComsumedDiamond = (isRoomOwnerPay() && (getDiamondNeed(m_nRoundLevel, getPayType()) > 0));
	if (isDismissed && isAlreadyComsumedDiamond && isOneRoundNormalEnd() == false)
	{
		Json::Value jsReq;
		jsReq["targetUID"] = m_nOwnerUID;
		jsReq["diamond"] = getDiamondNeed(m_nRoundLevel, getPayType());
		jsReq["roomID"] = getRoomID();
		jsReq["reason"] = 1;
		auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, m_nOwnerUID, eAsync_GiveBackDiamond, jsReq);
		LOGFMTD("room id = %u dissmiss give back uid = %u diamond = %u", getRoomID(), m_nOwnerUID, jsReq["diamond"].asUInt());
	}

	// find big wineer and pay room card 
	if (isWinerPay() && isOneRoundNormalEnd() && getDiamondNeed(m_nRoundLevel, getPayType()) > 0)
	{
		doProcessWinerPayRoomCard();
	}

	m_nPrivateRoomState = eState_RoomOvered;
	// delete self
	m_pRoomMgr->deleteRoom(getRoomID());
	//IPrivateRoom::doRoomGameOver(isDismissed);
}

void ThirteenPrivateRoom::onPlayerRotBanker(IGamePlayer* pPlayer, uint8_t nCoin) {
	m_nRotBankerPool += nCoin;
	getRoomRecorder()->addRotBankerPool(pPlayer->getUserUID(), nCoin);
	Json::Value jsMsg;
	jsMsg["pool"] = m_nRotBankerPool;
	sendRoomMsg(jsMsg, MSG_ROOM_THIRTEEN_RBPOOL_UPDATE);
}

uint32_t ThirteenPrivateRoom::isClubRoom() {
	return getClubID();
}

bool ThirteenPrivateRoom::onPlayerNetStateRefreshed(uint32_t nPlayerID, eNetState nState) {
	//LOGFMTE("player = %u, net state changed to %u", nPlayerID, nState);
	auto st = isEnterByUserID(nPlayerID);
	if (st) {
		//LOGFMTE("player = %u, net state changed to %u, session = %u", nPlayerID, nState, st->nSessionID);
		st->nState = nState;
	}
	return IPrivateRoom::onPlayerNetStateRefreshed(nPlayerID, nState);
}

void ThirteenPrivateRoom::onDismiss() {
	m_tCreateTimeLimit.setInterval(0);
}

bool ThirteenPrivateRoom::onPlayerEnter(stEnterRoomData* pEnterRoomPlayer) {

	pEnterRoomPlayer->nChip = 0;
	if (m_pRoom == nullptr)
	{
		LOGFMTE("why room is null ? ");
		return false;
	}
	auto sPlayer = isEnterByUserID(pEnterRoomPlayer->nUserUID);
	if (sPlayer) {
		//auto sPlayer = m_mStayPlayers[pEnterRoomPlayer->nUserUID];
		pEnterRoomPlayer->nChip = m_mStayPlayers[pEnterRoomPlayer->nUserUID]->nChip;
		sPlayer->nSessionID = pEnterRoomPlayer->nSessionID;
		sPlayer->nState = eNet_Online;
		sPlayer->nEnterClubID = pEnterRoomPlayer->nClubID;
	}
	else {
		stStayPlayer* stp = new stStayPlayer();
		stp->nUserUID = pEnterRoomPlayer->nUserUID;
		stp->nChip = 0;
		stp->nSessionID = pEnterRoomPlayer->nSessionID;
		stp->nEnterClubID = pEnterRoomPlayer->nClubID;
		m_mStayPlayers[stp->nUserUID] = stp;
	}
	if (m_pRoom->onPlayerEnter(pEnterRoomPlayer))
	{

	}
	return true;
}

bool ThirteenPrivateRoom::canPlayerSitDown(uint32_t nUserUID) {
	auto stg = isEnterByUserID(nUserUID);
	if (stg) {
		return stg->nChip >= ((ThirteenRoom*)getCoreRoom())->getDragInNeed();
	}
	return false;
}

void ThirteenPrivateRoom::onPlayerSitDown(IGameRoom* pRoom, IGamePlayer* pPlayer) {
	if (m_mStayPlayers.count(pPlayer->getUserUID())) {
		pPlayer->setChips(m_mStayPlayers[pPlayer->getUserUID()]->nChip);
	}
	else {
		stStayPlayer* stp = new stStayPlayer();
		stp->nUserUID = pPlayer->getUserUID();
		stp->nChip = 0;
		stp->nSessionID = pPlayer->getSessionID();
		m_mStayPlayers[stp->nUserUID] = stp;
		pPlayer->setChips(0);
	}
	m_mStayPlayers[pPlayer->getUserUID()]->isSitdown = true;
}

void ThirteenPrivateRoom::onPlayerDoLeaved(IGameRoom* pRoom, uint32_t nUserUID) {
	// tell data svr player leave ;
	auto sPlayer = isEnterByUserID(nUserUID);
	if (sPlayer) {
		sPlayer->reset();
	}
	Json::Value jsReqLeave;
	jsReqLeave["targetUID"] = nUserUID;
	jsReqLeave["roomID"] = getRoomID();
	jsReqLeave["port"] = m_pRoomMgr->getSvrApp()->getLocalSvrMsgPortType();
	auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
	pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nUserUID, eAsync_Inform_Player_LeavedRoom, jsReqLeave);
}

void ThirteenPrivateRoom::onPlayerTOut(uint32_t nUserUID) {
	auto sPlayer = isEnterByUserID(nUserUID);
	if (sPlayer) {
		sPlayer->isTOut = true;
	}
}

bool ThirteenPrivateRoom::onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID) {
	switch (nMsgType) {
	case MSG_PLAYER_SIT_DOWN:
	{
		auto sPlayer = isEnterBySession(nSessionID);
		if (sPlayer == nullptr) {
			Json::Value jsRet;
			//jsRet["ret"] = 4;
			//sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			sendMsgToPlayer(jsRet, MSG_ROOM_PLAYER_LEAVE, nSessionID);
			break;
		}
		else {
			return IPrivateRoom::onMsg(prealMsg, nMsgType, eSenderPort, nSessionID);
		}
	}
	break;
	case MSG_ROOM_THIRTEEN_DELAY_TIME:
	{
		if (isRoomGameOver()) {
			Json::Value jsMsg;
			jsMsg["ret"] = 1;
			sendMsgToPlayer(jsMsg, nMsgType, nSessionID);
			break;
		}
		if (prealMsg["uid"].isNull() || prealMsg["uid"].isUInt() == false) {
			Json::Value jsMsg;
			jsMsg["ret"] = 2;
			sendMsgToPlayer(jsMsg, nMsgType, nSessionID);
			break;
		}
		auto nUID = prealMsg["uid"].asUInt();
		auto nTime = prealMsg["time"].asUInt() == 30 ? 30 : 60;
		uint8_t nIdx = nTime == 30 ? 0 : 1;
		uint8_t nIdx_1 = m_nRoundLevel >> 4;
		uint16_t vDiamond[2][6] = { {120, 600, 900, 1500, 3000, 6000}, {180, 900, 1350, 2250, 4500, 9000} };
		uint32_t nDiamond = vDiamond[nIdx][nIdx_1];
		auto pApp = m_pRoomMgr->getSvrApp();
		Json::Value jsReq;
		jsReq["targetUID"] = nUID;
		jsReq["diamond"] = nDiamond;
		jsReq["roomID"] = getRoomID();

#ifdef _DEBUG
		nTime = 3;
#endif // _DEBUG

		pApp->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, nUID, eAsync_thirteen_delay_check_Diamond, jsReq, [pApp, nMsgType, nUID, nSessionID, nDiamond, nTime, this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
		{
			Json::Value jsRet;
			if (isTimeOut)
			{
				LOGFMTE(" request apply delay room time out uid = %u , can not delay room time", nUID);
				jsRet["ret"] = 7;
				sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				return;
			}

			uint8_t nReqRet = retContent["ret"].asUInt();
			uint8_t nRet = 0;
			do {
				if (0 != nReqRet)
				{
					nRet = 4;
					break;
				}

				if (isRoomGameOver()) {
					//Json::Value jsMsg;
					nRet = 1;
					//sendMsgToPlayer(jsMsg, nMsgType, nSessionID);
					break;
				}

				//LOGFMTE("before delay time is %f", m_tCreateTimeLimit.getDuringTime());
				m_tCreateTimeLimit.addInterval(nTime * 60);
				//LOGFMTE("after delay time is %f", m_tCreateTimeLimit.getDuringTime());
				Json::Value jsConsumDiamond;
				jsConsumDiamond["playerUID"] = nUID;
				jsConsumDiamond["diamond"] = nDiamond;
				jsConsumDiamond["roomID"] = getRoomID();
				jsConsumDiamond["reason"] = 1;
				pApp->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, nUID, eAsync_Consume_Diamond, jsConsumDiamond);
				LOGFMTD("user uid = %u delay room time do comuse diamond = %u room id = %u", nUID, nDiamond, getRoomID());

			} while (0);

			jsRet["ret"] = nRet;
			if (nRet == 0) {
				jsRet["leftTime"] = (int32_t)m_tCreateTimeLimit.getDuringTime();
				jsRet["time"] = (int32_t)m_tCreateTimeLimit.getInterval();
				sendRoomMsg(jsRet, nMsgType, nSessionID);
			}
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
		});
	}
	break;
	case MSG_ROOM_THIRTEEN_DISMISS_ROOM:
	{
		if (prealMsg["uid"].isNull() || prealMsg["uid"].isUInt() == false) {
			Json::Value jsMsg;
			jsMsg["ret"] = 1;
			sendMsgToPlayer(jsMsg, nMsgType, nSessionID);
			break;
		}
		auto nUID = prealMsg["uid"].asUInt();
		if (nUID != m_nOwnerUID) {
			Json::Value jsMsg;
			jsMsg["ret"] = 2;
			sendMsgToPlayer(jsMsg, nMsgType, nSessionID);
			break;
		}
		if (m_nOverType == ROOM_OVER_TYPE_TIME) {
			m_tCreateTimeLimit.setInterval(0);
		}
		else {
			m_nLeftRounds = 0;
		}
		Json::Value jsMsg;
		jsMsg["ret"] = 0;
		sendMsgToPlayer(jsMsg, nMsgType, nSessionID);
	}
	break;
	case MSG_ROOM_THIRTEEN_BOARD_GAME_RECORD:
	{
		if (!getRoomRecorder() || getRoomRecorder()->getRoundCnt() == 0) {
			Json::Value jsMsg;
			jsMsg["ret"] = 1;
			sendMsgToPlayer(jsMsg, nMsgType, nSessionID);
			break;
		}

		auto st = isEnterBySession(nSessionID);
		if (st == nullptr) {
			Json::Value jsMsg;
			jsMsg["ret"] = 2;
			sendMsgToPlayer(jsMsg, nMsgType, nSessionID);
			break;
		}

		std::vector<std::shared_ptr<ISingleRoundRecorder>> vRecorder;
		getRoomRecorder()->getPlayerSingleRecorder(st->nUserUID, vRecorder);
		if (vRecorder.size() == 0) {
			Json::Value jsMsg;
			jsMsg["ret"] = 3;
			sendMsgToPlayer(jsMsg, nMsgType, nSessionID);
			break;
		}

		int16_t nIdx = 0;
		if (prealMsg["idx"].isNull() || prealMsg["idx"].isInt() == false) {
			LOGFMTE("request board game record idx type error? ");
			nIdx = -1;
		}
		else {
			nIdx = prealMsg["idx"].asInt();
		}

		if ((int16_t)-1 == nIdx) {
			nIdx = vRecorder.size() - 1;
		}

		if (nIdx >= vRecorder.size()) {
			Json::Value jsMsg;
			jsMsg["ret"] = 4;
			sendMsgToPlayer(jsMsg, nMsgType, nSessionID);
			break;
		}

		Json::Value jsMsg, jsDetail;
		jsMsg["ret"] = 0;
		jsMsg["idx"] = nIdx;
		auto recorder = vRecorder.at(nIdx);
		recorder->toJson(jsDetail);
		jsMsg["bankerUID"] = recorder->getBankerUID();
		jsMsg["rotBanker"] = recorder->isRotBanker() ? 1 : 0;
		jsMsg["detail"] = jsDetail;
		sendMsgToPlayer(jsMsg, nMsgType, nSessionID);
	}
	break;
	case MSG_ROOM_REQUEST_THIRTEEN_ROOM_INFO:
	{
		sendBssicRoomInfo(nSessionID);
	}
	break;
	case MSG_ROOM_THIRTEEN_REAL_TIME_RECORD :
	{
		uint32_t tIdx = 0;
		uint32_t pIdx = 0;
		std::vector<stStayPlayer*> vsPlayers;
		for (auto ref : m_mStayPlayers) {
			vsPlayers.push_back(ref.second);
		}
		while (tIdx < vsPlayers.size()) {
			Json::Value jsMsg, jsDetails;
			jsMsg["idx"] = pIdx;
			uint8_t cIdx = 0;
			while (cIdx < 10) {
				if (tIdx >= vsPlayers.size()) {
					break;
				}
				auto st = vsPlayers.at(tIdx);
				tIdx++;
				if (st->isJoin == 0 && st->isDragIn == false) {
					continue;
				}
				cIdx++;
				Json::Value jsDetail;
				jsDetail["uid"] = st->nUserUID;
				auto pPlayer = getCoreRoom()->getPlayerByUID(st->nUserUID);
				if (pPlayer) {
					jsDetail["chip"] = pPlayer->getChips();
				}
				else {
					jsDetail["chip"] = st->nChip;
				}
				jsDetail["drag"] = st->nAllWrag;
				jsDetail["round"] = st->isJoin;
				jsDetails[jsDetails.size()] = jsDetail;
			}



			/*for (; tIdx < tIdx + 10; tIdx++) {
				if (tIdx >= vsPlayers.size()) {
					break;
				}
				Json::Value jsDetail;
				auto st = vsPlayers.at(tIdx);
				if (st->isJoin == false) {
					continue;
				}
				jsDetail["uid"] = st->nUserUID;
				auto pPlayer = getCoreRoom()->getPlayerByUID(st->nUserUID);
				if (pPlayer) {
					jsDetail["chip"] = pPlayer->getChips();
				}
				else {
					jsDetail["chip"] = st->nChip;
				}
				jsDetail["drag"] = st->nAllWrag;
				jsDetails[jsDetails.size()] = jsDetail;
			}*/
			jsMsg["detail"] = jsDetails;
			pIdx++;
			sendMsgToPlayer(jsMsg, nMsgType, nSessionID);
		}
	}
	break;
	default :
	{
		return IPrivateRoom::onMsg(prealMsg, nMsgType, eSenderPort, nSessionID);
	}
	}
	return true;
}

void ThirteenPrivateRoom::sendBssicRoomInfo(uint32_t nSessionID) {
	Json::Value jsMsg;
	jsMsg["ret"] = 0;
	jsMsg["roomID"] = getRoomID();
	if (m_nOverType == ROOM_OVER_TYPE_TIME) {
		jsMsg["leftTime"] = (int32_t)m_tCreateTimeLimit.getDuringTime();
	}
	jsMsg["playCnt"] = getCoreRoom()->getPlayerCnt();
	jsMsg["playerCnt"] = getPlayerCnt();
	jsMsg["opts"] = getOpts();
	sendMsgToPlayer(jsMsg, MSG_ROOM_REQUEST_THIRTEEN_ROOM_INFO, nSessionID);
}

ThirteenPrivateRoom::stStayPlayer* ThirteenPrivateRoom::isEnterBySession(uint32_t nSessionID) {
	for (auto ref : m_mStayPlayers) {
		auto refs = ref.second;
		if (refs->nSessionID == nSessionID) {
			return refs;
		}
	}
	return nullptr;
}

ThirteenPrivateRoom::stStayPlayer* ThirteenPrivateRoom::isEnterByUserID(uint32_t nUserID) {
	if (m_mStayPlayers.count(nUserID)) {
		return m_mStayPlayers[nUserID];
	}
	return nullptr;
}

//void ThirteenPrivateRoom::setCurrentPointer(IGameRoom* pRoom) {
//	if (std::find(m_vPRooms.begin(), m_vPRooms.end(), pRoom) == m_vPRooms.end()) {
//		assert(0 && "invalid argument");
//	}
//	else {
//		if (dynamic_cast<ThirteenRoom*>(pRoom)) {
//			m_pRoom = (GameRoom*)pRoom;
//		}
//		else {
//			assert(0 && "invalid argument");
//		}
//	}
//}