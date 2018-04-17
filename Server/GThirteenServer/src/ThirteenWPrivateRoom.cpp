#include "ThirteenWPrivateRoom.h"
#include "Thirteen\ThirteenRoom.h"
#include "IGamePlayer.h"
#include "IGameRoomManager.h"
#include "AsyncRequestQuene.h"
#include "../ServerCommon/ISeverApp.h"
#include "stEnterRoomData.h"

ThirteenWPrivateRoom::~ThirteenWPrivateRoom() {
	for (auto& ref : m_vPRooms) {
		if (ref) {
			if (m_pRoom == ref) {
				m_pRoom = nullptr;
			}
			delete ref;
			ref = nullptr;
		}
	}
	m_vPRooms.clear();
}

bool ThirteenWPrivateRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts) {
	if (IPrivateRoom::init(pRoomMgr, nSeialNum, nRoomID, nSeatCnt, vJsOpts)) {
		m_isForbitEnterRoomWhenStarted = vJsOpts["forbidJoin"].asUInt() == 1;
		m_nAutoOpenCnt = vJsOpts["starGame"].asUInt();
		m_nMaxCnt = vJsOpts["maxCnt"].isUInt() ? vJsOpts["maxCnt"].asUInt() : 0;

		m_nStartTime = vJsOpts["startTime"].isUInt() ? vJsOpts["startTime"].asUInt() : 0;
		m_bNeedVerify = vJsOpts["needVerify"].isUInt() ? vJsOpts["needVerify"].asBool() : false;
		m_nInitialCoin = vJsOpts["initialCoin"].isUInt() ? vJsOpts["initialCoin"].asUInt() : 0;
		m_nRiseBlindTime = vJsOpts["rbt"].isUInt() ? vJsOpts["rbt"].asUInt() : 0;
		m_nRebuyLevel = vJsOpts["rebuyLevel"].isUInt() ? vJsOpts["rebuyLevel"].asUInt() : 0;
		m_nRebuyTime = vJsOpts["rebuyTime"].isUInt() ? vJsOpts["rebuyTime"].asUInt() : 0;
		m_nEnterFee = vJsOpts["enterFee"].isUInt() ? vJsOpts["enterFee"].asUInt() : 0;
		m_nDelayEnterLevel = vJsOpts["delayEnter"].isUInt() ? vJsOpts["delayEnter"].asUInt() : 0;

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

		m_ptrRoomRecorder = getCoreRoom()->createRoomRecorder();
		m_ptrRoomRecorder->init(nSeialNum, nRoomID, getCoreRoom()->getRoomType(), vJsOpts["uid"].asUInt(), vJsOpts);
		m_ptrRoomRecorder->setClubID(m_nClubID);
		m_ptrRoomRecorder->setLeagueID(m_nLeagueID);

		((ThirteenRoom*)m_pRoom)->signIsWaiting();
		m_vPRooms.push_back(m_pRoom);
		return true;
	}
	return false;
}

void ThirteenWPrivateRoom::setCurrentPointer(IGameRoom* pRoom) {
	if (std::find(m_vPRooms.begin(), m_vPRooms.end(), pRoom) == m_vPRooms.end()) {
		assert(0 && "invalid argument");
	}
	else {
		if (dynamic_cast<ThirteenRoom*>(pRoom)) {
			m_pRoom = (GameRoom*)pRoom;
		}
		else {
			assert(0 && "invalid argument");
		}
	}
}

void ThirteenWPrivateRoom::packRoomInfo(Json::Value& jsRoomInfo) {
	IPrivateRoom::packRoomInfo(jsRoomInfo);
}

bool ThirteenWPrivateRoom::onPlayerEnter(stEnterRoomData* pEnterRoomPlayer) {
	if (isRoomStarted()) {
		auto sPlayer = isEnterByUserID(pEnterRoomPlayer->nUserUID);
		if (sPlayer) {
			sPlayer->nSessionID = pEnterRoomPlayer->nSessionID;
			if ((uint32_t)-1 == sPlayer->nCurInIdx) {
				if (sPlayer->nChip) {
					packTempRoomInfoToPlayer(pEnterRoomPlayer);
					return false;
				}
				else {
					//观战TODO
					return enterRoomToWatch(pEnterRoomPlayer);
				}
			}
			else {
				if (sPlayer->nCurInIdx < m_vPRooms.size()) {
					m_pRoom = m_vPRooms[sPlayer->nCurInIdx];
					if (m_pRoom->onPlayerEnter(pEnterRoomPlayer)) {

					}
					return true;
				}
				else {
					sPlayer->nCurInIdx = -1;
					if (sPlayer->nChip) {
						packTempRoomInfoToPlayer(pEnterRoomPlayer);
						return false;
					}
					else {
						//观战TODO
						return enterRoomToWatch(pEnterRoomPlayer);
					}
				}
			}
		}
		else {
			//观战TODO
			return enterRoomToWatch(pEnterRoomPlayer);
		}
	}
	return false;
	//return IPrivateRoom::onPlayerEnter(pEnterRoomPlayer);
}

uint8_t ThirteenWPrivateRoom::checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer) {
	if (isRoomStarted()) {
		if (isAAPay()) {
			return 8;
		}

		if (isWinerPay()) {
			return 9;
		}

		if (isRoomFull()) {
			return 10;
		}

		/*auto sPlayer = isEnterByUserID(pEnterRoomPlayer->nUserUID);
		if (sPlayer && sPlayer->isTOut) {
			return 11;
		}*/
		return 0;
	}
	return 12;
}


uint8_t ThirteenWPrivateRoom::getInitRound(uint8_t nLevel) {
	return 1;
}

bool ThirteenWPrivateRoom::canStartGame(IGameRoom* pRoom) {
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

	bool canStart = nCnt > 1;

	//if (m_isOpen/* && m_nAutoOpenCnt > 0*/)
	//{
	//	canStart = nCnt > 1;
	//}
	//else {
	//	canStart = nCnt >= m_nAutoOpenCnt;
	//}

	if (canStart)
	{
		m_isOpen = true;
		Json::Value js;
		pRoom->sendRoomMsg(js, MSG_ROOM_DO_OPEN);
		LOGFMTI(" room id = %u auto set open", getRoomID());
	}

	return canStart;
}

void ThirteenWPrivateRoom::onGameDidEnd(IGameRoom* pRoom) {
	auto nCnt = ((ThirteenRoom*)pRoom)->getSeatCnt();
	for (uint8_t nIdx = 0; nIdx < nCnt; ++nIdx) {
		auto pPlayer = ((ThirteenRoom*)pRoom)->getPlayerByIdx(nIdx);
		if (!pPlayer || pPlayer->haveState(eRoomPeer_StayThisRound) == false)
		{
			//LOGFMTE( "player is null , comuse diamond idx = %u , room id = %u",nIdx , getRoomID() );
			continue;
		}
		auto stg = isEnterByUserID(pPlayer->getUserUID());
		if (stg) {
			stg->isJoin += 1;
		}
	}

	// check room over
	if (0 == m_nLeftRounds)
	{
		doRoomGameOver(false);
	}
}

bool ThirteenWPrivateRoom::canPlayerSitDown(uint32_t nUserUID) {
	auto stw = isEnterByUserID(nUserUID);
	if (stw) {
		return stw->nChip > 0;
	}
	return false;
}

void ThirteenWPrivateRoom::onPlayerSitDown(IGameRoom* pRoom, IGamePlayer* pPlayer) {
	auto stw = isEnterByUserID(pPlayer->getUserUID());
	if (stw) {
		pPlayer->setChips(stw->nChip);
	}
}

void ThirteenWPrivateRoom::onPlayerDoLeaved(IGameRoom* pRoom, uint32_t nUserUID) {
	Json::Value jsReqLeave;
	jsReqLeave["targetUID"] = nUserUID;
	jsReqLeave["roomID"] = getRoomID();
	jsReqLeave["port"] = m_pRoomMgr->getSvrApp()->getLocalSvrMsgPortType();
	auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
	pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nUserUID, eAsync_Inform_Player_LeavedRoom, jsReqLeave);
}

bool ThirteenWPrivateRoom::onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID) {
	switch (nMsgType) {
	case MSG_PLAYER_SIT_DOWN:
	{
		Json::Value jsRet;
		jsRet["ret"] = 10;
		sendMsgToPlayer(jsRet, nMsgType, nSessionID);
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
		m_nLeftRounds = 0;
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
	case MSG_ROOM_THIRTEEN_REAL_TIME_RECORD:
	{
		uint32_t tIdx = 0;
		uint32_t pIdx = 0;
		std::vector<stwStayPlayer*> vsPlayers;
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
			jsMsg["detail"] = jsDetails;
			pIdx++;
			sendMsgToPlayer(jsMsg, nMsgType, nSessionID);
		}
	}
	break;
	default:
	{
		return IPrivateRoom::onMsg(prealMsg, nMsgType, eSenderPort, nSessionID);
	}
	}
	return true;
}

bool ThirteenWPrivateRoom::isRoomFull() {
	return false;
}