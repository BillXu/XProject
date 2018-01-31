#include "ThirteenPrivateRoom.h"
#include "Thirteen\ThirteenRoom.h"
#include "IGamePlayer.h"
#include "IGameRoomState.h"
#include "IGameRoomManager.h"
#include "AsyncRequestQuene.h"
#include "../ServerCommon/ISeverApp.h"
//ThirteenPrivateRoom::~ThirteenPrivateRoom() {
//	for (auto& ref : m_vPRooms) {
//		if (ref) {
//			if (m_pRoom == ref) {
//				m_pRoom = nullptr;
//			}
//			delete ref;
//			ref = nullptr;
//		}
//	}
//	m_vPRooms.clear();
//}

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
		return true;
	}
	
	return false;
}

void ThirteenPrivateRoom::packRoomInfo(Json::Value& jsRoomInfo) {
	IPrivateRoom::packRoomInfo(jsRoomInfo);
	if (m_nOverType == ROOM_OVER_TYPE_TIME) {
		jsRoomInfo["leftTime"] = (int32_t)m_tCreateTimeLimit.getDuringTime();
	}
}

uint8_t ThirteenPrivateRoom::checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)
{
	if (m_isForbitEnterRoomWhenStarted && isRoomStarted() )
	{
		return 7;
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
	uint8_t nTypeLevel = (nLevel << 4) >> 4;
	switch (m_nOverType) {
	case ROOM_OVER_TYPE_TIME:
	{
		uint8_t vJun[] = {10, 15, 20, 30, 60};
		if (nTypeLevel >= sizeof(vJun) / sizeof(uint8_t))
		{
			LOGFMTE("invalid level type = %u", nLevel);
			nTypeLevel = 0;
		}
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
	Json::Value jsArrayPlayers;
	auto nCnt = getCoreRoom()->getSeatCnt();
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
	}

	Json::Value jsMsg;
	jsMsg["dismissID"] = m_nApplyDismissUID;
	jsMsg["result"] = jsArrayPlayers;
	sendRoomMsg(jsMsg, MSG_ROOM_GAME_OVER );
}

bool ThirteenPrivateRoom::canStartGame(IGameRoom* pRoom)
{
	if ( m_isOpen == false && m_nAutoOpenCnt > 0)
	{
		uint8_t nCnt = 0;
		auto pThirteenRoom = ((ThirteenRoom*)pRoom);
		for (uint8_t nIdx = 0; nIdx < pThirteenRoom->getSeatCnt(); ++nIdx)
		{
			if (pThirteenRoom->getPlayerByIdx(nIdx))
			{
				++nCnt;
			}
		}

		m_isOpen = nCnt >= m_nAutoOpenCnt;
		if (m_isOpen)
		{
			m_isOpen = true;
			Json::Value js;
			sendRoomMsg(js, MSG_ROOM_DO_OPEN);
			LOGFMTI(" room id = %u auto set open", getRoomID());
		}
	}

	return IPrivateRoom::canStartGame(pRoom);
}

void ThirteenPrivateRoom::onPlayerWillStandUp(IGameRoom* pRoom, IGamePlayer* pPlayer) {
	if (m_mStayPlayers.count(pPlayer->getUserUID())) {
		m_mStayPlayers[pPlayer->getUserUID()].nChip = pPlayer->getChips();
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

	// check room over
	if (0 == m_nLeftRounds)
	{
		doRoomGameOver(false);
	}
}

void ThirteenPrivateRoom::onPlayerApplyDragIn(uint16_t nCnt) {
	//getCoreRoom()->getPlayerByUID();
}

bool ThirteenPrivateRoom::onPlayerDragIn(uint32_t nUserID, uint32_t nAmount) {
	if (nAmount == 0) {
		return false;
	}
	if (m_mStayPlayers.count(nUserID) == 0) {
		return false;
	}
	if (((ThirteenRoom*)getCoreRoom())->onPlayerDragIn(nUserID, nAmount)) {
		m_mStayPlayers[nUserID].nChip = getCoreRoom()->getPlayerByUID(nUserID)->getChips();
	}
}

bool ThirteenPrivateRoom::doDeleteRoom() {
	if (IPrivateRoom::doDeleteRoom()) {
		if (m_nLeagueID) {
			Json::Value jsReqInfo;
			jsReqInfo["leagueID"] = m_nLeagueID;
			jsReqInfo["roomID"] = getRoomID();
			auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
			pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, m_nLeagueID, eAsync_league_or_club_DeleteRoom, jsReqInfo);
		}
		else if (m_nClubID) {
			Json::Value jsReqInfo;
			jsReqInfo["clubID"] = m_nClubID;
			jsReqInfo["roomID"] = getRoomID();
			auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
			pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, m_nClubID, eAsync_league_or_club_DeleteRoom, jsReqInfo);
		}
		return true;
	}
	return false;
}

bool ThirteenPrivateRoom::isRoomGameOver() {
	switch (m_nOverType) {
	case ROOM_OVER_TYPE_TIME:
	{
		return m_nLeftRounds < 1;
	}
	case ROOM_OVER_TYPE_ROUND:
	{
		return m_nLeftRounds <= 1;
	}
	}
	return true;
}

bool ThirteenPrivateRoom::onPlayerEnter(stEnterRoomData* pEnterRoomPlayer) {

	pEnterRoomPlayer->nChip = 0;
	if (m_pRoom == nullptr)
	{
		LOGFMTE("why room is null ? ");
		return false;
	}
	if (m_mStayPlayers.count(pEnterRoomPlayer->nUserUID)) {
		pEnterRoomPlayer->nChip = m_mStayPlayers[pEnterRoomPlayer->nUserUID].nChip;
	}
	else {
		stStayPlayer stp;
		stp.nUserUID = pEnterRoomPlayer->nUserUID;
		stp.nChip = 0;
		m_mStayPlayers[stp.nUserUID] = stp;
	}
	if (m_pRoom->onPlayerEnter(pEnterRoomPlayer))
	{

	}
	return true;
}

void ThirteenPrivateRoom::onPlayerSitDown(IGameRoom* pRoom, IGamePlayer* pPlayer) {
	if (m_mStayPlayers.count(pPlayer->getUserUID())) {
		pPlayer->setChips(m_mStayPlayers[pPlayer->getUserUID()].nChip);
	}
	else {
		pPlayer->setChips(0);
	}
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