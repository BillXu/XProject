#include "ThirteenWPrivateRoom.h"
#include "Thirteen\ThirteenRoom.h"
#include "IGamePlayer.h"
#include "IGameRoomManager.h"
#include "AsyncRequestQuene.h"
#include "../ServerCommon/ISeverApp.h"
#include "stEnterRoomData.h"
#include "IGameRoomState.h"
#include <time.h>

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
		m_tAutoDismissTimer.canncel();
		m_isForbitEnterRoomWhenStarted = vJsOpts["forbidJoin"].asUInt() == 1;
		m_nAutoOpenCnt = vJsOpts["starGame"].asUInt();
		m_nMaxCnt = vJsOpts["maxCnt"].isUInt() ? vJsOpts["maxCnt"].asUInt() : 0;

		m_nStartTime = vJsOpts["startTime"].isUInt() ? vJsOpts["startTime"].asUInt() : 0;
		m_bNeedVerify = vJsOpts["needVerify"].isUInt() ? vJsOpts["needVerify"].asBool() : true;
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

		initLevelInfo();
		m_tMTTBlindRise.reset();
		m_tMTTBlindRise.setIsAutoRepeat(true);
		m_tMTTBlindRise.setInterval(m_nRiseBlindTime * 60);
		m_tMTTBlindRise.setCallBack([this](CTimer*p, float f) {
			/*m_tInvokerTime = 0;
			m_nApplyDismissUID = 0;
			LOGFMTI("system auto dismiss room id = %u , owner id = %u", getRoomID(), m_nOwnerUID);
			doRoomGameOver(true);*/
			m_nCurBlind++;
			LOGFMTI("system auto rise blind to %u room id = %u , owner id = %u", m_nCurBlind, getRoomID(), m_nOwnerUID);
		});
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
	bool isOpen = m_tMTTBlindRise.isRunning();
	jsRoomInfo["curBlind"] = m_nCurBlind;
	jsRoomInfo["riseTime"] = (int32_t)m_tMTTBlindRise.getDuringTime();
	jsRoomInfo["dragInCnt"] = getPlayerCnt();
	jsRoomInfo["aliveCnt"] = getAliveCnt();
	jsRoomInfo["isOpen"] = isOpen ? 1 : 0;

	uint32_t nUserID = jsRoomInfo["uid"].isUInt() ? jsRoomInfo["uid"].asUInt() : 0;
	auto sPlayer = (stwStayPlayer*)isEnterByUserID(nUserID);
	if (sPlayer && sPlayer->isDragIn) {
		if (isOpen && sPlayer->nOutGIdx) {
			jsRoomInfo["playerState"] = MTT_PLAYER_TOUT;
		}
		else {
			jsRoomInfo["playerState"] = MTT_PLAYER_DRAGIN;
		}
	}
	else {
		jsRoomInfo["playerState"] = MTT_PLAYER_NOT_DRAGIN;
	}
}

bool ThirteenWPrivateRoom::onPlayerEnter(stEnterRoomData* pEnterRoomPlayer) {
	auto sPlayer = (stwStayPlayer*)isEnterByUserID(pEnterRoomPlayer->nUserUID);
	if (sPlayer) {
		pEnterRoomPlayer->nChip = sPlayer->nChip;
		sPlayer->nSessionID = pEnterRoomPlayer->nSessionID;
		if ((uint32_t)-1 != sPlayer->nCurInIdx && sPlayer->nCurInIdx < m_vPRooms.size()) {
			m_pRoom = m_vPRooms[sPlayer->nCurInIdx];
			if (m_pRoom->onPlayerEnter(pEnterRoomPlayer)) {
				m_pRoom->onPlayerSetNewSessionID(pEnterRoomPlayer->nUserUID, pEnterRoomPlayer->nSessionID);
				return true;
			}
			else {
				sPlayer->tempLeave();
			}
		}
	}
	return enterRoomToWatch(pEnterRoomPlayer);

	//if (isRoomStarted()) {
	//	auto sPlayer = (stwStayPlayer*)isEnterByUserID(pEnterRoomPlayer->nUserUID);
	//	if (sPlayer) {
	//		pEnterRoomPlayer->nChip = sPlayer->nChip;
	//		sPlayer->nSessionID = pEnterRoomPlayer->nSessionID;
	//		if ((uint32_t)-1 == sPlayer->nCurInIdx) {
	//			//if (sPlayer->nChip) {
	//			//	//packTempRoomInfoToPlayer(pEnterRoomPlayer);
	//			//	//return false;
	//			//}
	//			//else {
	//			//	//观战TODO
	//			//	return enterRoomToWatch(pEnterRoomPlayer);
	//			//}
	//			return enterRoomToWatch(pEnterRoomPlayer);
	//		}
	//		else {
	//			if (sPlayer->nCurInIdx < m_vPRooms.size()) {
	//				m_pRoom = m_vPRooms[sPlayer->nCurInIdx];
	//				if (m_pRoom->onPlayerEnter(pEnterRoomPlayer)) {

	//				}
	//				return true;
	//			}
	//			else {
	//				sPlayer->tempLeave();
	//				return enterRoomToWatch(pEnterRoomPlayer);
	//				//sPlayer->nCurInIdx = -1;
	//				//if (sPlayer->nChip) {
	//				//	packTempRoomInfoToPlayer(pEnterRoomPlayer);
	//				//	return false;
	//				//}
	//				//else {
	//				//	//观战TODO
	//				//	return enterRoomToWatch(pEnterRoomPlayer);
	//				//}
	//			}
	//		}
	//	}
	//	else {
	//		//观战TODO
	//		return enterRoomToWatch(pEnterRoomPlayer);
	//	}
	//}
	//return false;
	//return IPrivateRoom::onPlayerEnter(pEnterRoomPlayer);
}

void ThirteenWPrivateRoom::doPlayerEnter(IGameRoom* pRoom, uint32_t nUserUID) {
	if (m_mStayPlayers.count(nUserUID)) {
		auto stg = (stwStayPlayer*)m_mStayPlayers[nUserUID];
		for (uint16_t i = 0; i < m_vPRooms.size(); i++) {
			if (m_vPRooms[i] == pRoom) {
				stg->nCurInIdx = i;
				//stg->bLeaved = false;
				return;
			}
		}
	}
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

void ThirteenWPrivateRoom::onPreGameDidEnd(IGameRoom* pRoom) {
	m_pRoom = (ThirteenRoom*)pRoom;

	if (m_isOneRoundNormalEnd == false) {
		m_isOneRoundNormalEnd = true;
	}

	auto nCnt = ((ThirteenRoom*)pRoom)->getSeatCnt();
	for (uint8_t nIdx = 0; nIdx < nCnt; ++nIdx) {
		auto pPlayer = ((ThirteenRoom*)pRoom)->getPlayerByIdx(nIdx);
		if (pPlayer && pPlayer->haveState(eRoomPeer_StayThisRound)) {
			auto stg = (stwStayPlayer*)isEnterByUserID(pPlayer->getUserUID());
			if (stg) {
				stg->nChip = pPlayer->getChips();
				stg->isJoin += 1;
				if (pPlayer->getChips() < ((ThirteenRoom*)getCoreRoom())->getDragInNeed()) {
					uint32_t tOutTime = 0;
					uint32_t nOutIdx = getCoreRoom()->getRoomRecorder()->getRoundCnt() + 1;
					getRoomRecorder()->onMTTPlayerOut(pPlayer->getUserUID(), tOutTime, nOutIdx);
					stg->tOutTime = tOutTime;
					stg->nOutGIdx = nOutIdx;
				}
			}
		}
	}
}

void ThirteenWPrivateRoom::onGameDidEnd(IGameRoom* pRoom) {
	m_pRoom = (ThirteenRoom*)pRoom;
	if (m_nCurBlind > m_nRebuyLevel && m_nCurBlind > m_nDelayEnterLevel && getAliveCnt() < 2) {
		onDismiss();
	}

	auto nCnt = ((ThirteenRoom*)pRoom)->getSeatCnt();
	uint8_t nCnJoinCnt = 0;
	for (uint8_t nIdx = 0; nIdx < nCnt; ++nIdx) {
		auto pPlayer = ((ThirteenRoom*)pRoom)->getPlayerByIdx(nIdx);
		if (pPlayer) {
			if (pPlayer->getChips() >= ((ThirteenRoom*)pRoom)->getDragInNeed()) {
				nCnJoinCnt++;
			}
		}
	}
	if (isRoomGameOver()) {
		doSendRoomGameOverInfoToClient(false);
		((ThirteenRoom*)pRoom)->clearRoom();
		((ThirteenRoom*)pRoom)->signIsWaiting();
	}
	else {
		if ((nCnJoinCnt < 3 && getEmptySeatCnt(pRoom) >= nCnJoinCnt) || nCnJoinCnt < 2) {
			((ThirteenRoom*)pRoom)->clearRoom();
			((ThirteenRoom*)pRoom)->signIsWaiting();
		}
		else if (m_bNeedSplitRoom) {
			((ThirteenRoom*)pRoom)->clearRoom();
			((ThirteenRoom*)pRoom)->signIsWaiting();
			m_bNeedSplitRoom = false;
		}
	}
}

void ThirteenWPrivateRoom::onPlayerApplyDragIn(uint32_t nUserUID, uint32_t nClubID) {
	//TODO
}

void ThirteenWPrivateRoom::onPlayerStandedUp(IGameRoom* pRoom, uint32_t nUserUID) {
	auto sPlayer = (stwStayPlayer*)isEnterByUserID(nUserUID);
	if (sPlayer) {
		sPlayer->bLeaved = true;
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
	auto stw = (stwStayPlayer*)isEnterByUserID(pPlayer->getUserUID());
	if (stw) {
		pPlayer->setChips(stw->nChip);
		stw->bLeaved = false;
	}
	else {
		stwStayPlayer* stp = new stwStayPlayer();
		stp->nUserUID = pPlayer->getUserUID();
		stp->nChip = 0;
		stp->nSessionID = pPlayer->getSessionID();
		stw->bLeaved = false;
		m_mStayPlayers[stp->nUserUID] = stp;
		pPlayer->setChips(0);
	}
}

void ThirteenWPrivateRoom::onPlayerDoLeaved(IGameRoom* pRoom, uint32_t nUserUID) {
	auto sPlayer = (stwStayPlayer*)isEnterByUserID(nUserUID);
	if (sPlayer) {
		sPlayer->leave();
		if (pRoom) {
			pRoom->onPlayerNetStateRefreshed(nUserUID, eNet_WaitReconnect);
		}
		else {
			LOGFMTE("mtt game request leave player but proom is null? ");
		}
	}

	Json::Value jsReqLeave;
	jsReqLeave["targetUID"] = nUserUID;
	jsReqLeave["roomID"] = getRoomID();
	jsReqLeave["port"] = m_pRoomMgr->getSvrApp()->getLocalSvrMsgPortType();
	auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
	pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nUserUID, eAsync_Inform_Player_LeavedRoom, jsReqLeave);
}

void ThirteenWPrivateRoom::onPlayerTempLeaved(IGameRoom* pRoom, stEnterRoomData* pEnterRoomPlayer) {
	auto sPlayer = (stwStayPlayer*)isEnterByUserID(pEnterRoomPlayer->nUserUID);
	if (sPlayer) {
		sPlayer->tempLeave();
	}
	if (isRoomGameOver()) {
		onPlayerDoLeaved(pRoom, pEnterRoomPlayer->nUserUID);
	}
	else {
		enterRoomToWatch(pEnterRoomPlayer);
	}
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
		uint32_t nUserID = prealMsg["uid"].isUInt() ? prealMsg["uid"].asUInt() : 0;
		sendBssicRoomInfo(nSessionID, nUserID);
	}
	break;
	case MSG_ROOM_THIRTEEN_REAL_TIME_RECORD:
	{
		uint32_t tIdx = 0;
		uint32_t pIdx = 0;
		std::vector<stwStayPlayer*> vsPlayers;
		for (auto ref : m_mStayPlayers) {
			vsPlayers.push_back((stwStayPlayer*)ref.second);
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
				jsDetail["outTime"] = st->tOutTime;
				//jsDetail["outRound"] = st->nOutGIdx;
				jsDetails[jsDetails.size()] = jsDetail;
			}
			jsMsg["detail"] = jsDetails;
			pIdx++;
			sendMsgToPlayer(jsMsg, nMsgType, nSessionID);
		}
	}
	break;
	case MSG_ROOM_THIRTEEN_APPLAY_DRAG_IN:
	{
		Json::Value jsRet;
		/*if (((ThirteenRoom*)m_pRoom)->isClubRoom() == 0) {
			jsRet["ret"] = 10;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("Drag in error this room can not drag in? player sessionID = %u", nSessionID);
			return true;
		}
		if (m_tMTTBlindRise.isRunning() && m_nCurBlind > m_nRebuyLevel) {
			jsRet["ret"] = 11;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("Drag in error this room can not drag in? player sessionID = %u", nSessionID);
			return true;
		}*/

		auto nAmount = prealMsg["amount"].asUInt();
		auto pPlayer = (stwStayPlayer*)isEnterBySession(nSessionID);
		if (pPlayer == nullptr) {
			uint32_t nUserID = prealMsg["uid"].isUInt() ? prealMsg["uid"].asUInt() : 0;
			if (nUserID) {
				pPlayer = (stwStayPlayer*)isEnterByUserID(nUserID);
				if (pPlayer == nullptr) {
					jsRet["ret"] = 1;
					sendMsgToPlayer(jsRet, nMsgType, nSessionID);
					LOGFMTE("you are not in this room how to drag in? session id = %u", nSessionID);
					return true;
				}
			}
			else {
				jsRet["ret"] = 1;
				sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("you are not in this room how to drag in? session id = %u", nSessionID);
				return true;
			}
			
		}

		/*if (pPlayer == nullptr || pPlayer->bWaitDragIn)
		{
			jsRet["ret"] = 1;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("you are not in this room how to drag in? session id = %u", nSessionID);
			return true;
		}
		if (pPlayer->isDragIn && pPlayer->nChip >= ((ThirteenRoom*)getCoreRoom())->getDragInNeed()) {
			jsRet["ret"] = 14;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("Drag in error player do not need drag in? player sessionID = %u", nSessionID);
			return true;
		}
		if (pPlayer->isDragIn == false && m_tMTTBlindRise.isRunning() && m_nCurBlind > m_nDelayEnterLevel) {
			jsRet["ret"] = 12;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("Drag in error this room can not drag in? player sessionID = %u", nSessionID);
			return true;
		}
		if (m_nRebuyTime && pPlayer->isDragIn && pPlayer->nRebuyTime >= m_nRebuyTime) {
			jsRet["ret"] = 13;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("Drag in error this room can not drag in? player sessionID = %u", nSessionID);
			return true;
		}*/
		auto tRet = canPlayerDragIn(pPlayer->nUserUID);
		if (tRet) {
			jsRet["ret"] = tRet;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("Drag in error with ret = %u? player uid = %u", tRet, pPlayer->nUserUID);
			return true;
		}
		if (nAmount == 0) {
			jsRet["ret"] = 2;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("Drag in amount is missing? player uid = %u", pPlayer->nUserUID);
			return true;
		}
		if (((ThirteenRoom*)m_pRoom)->checkDragInAmount(nAmount) == false) {
			jsRet["ret"] = 3;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("Drag in amount is wrong? player uid = %u, amount = %u", pPlayer->nUserUID, nAmount);
			return true;
		}
		auto nClubID = prealMsg["clubID"].asUInt();
		if (nClubID == 0) {
			jsRet["ret"] = 5;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("Drag in clubID is wrong? player uid = %u, amount = %u", pPlayer->nUserUID, nAmount);
			return true;
		}
		else {
			if (pPlayer->nClubID && nClubID != pPlayer->nClubID) {
				jsRet["ret"] = 6;
				sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("Drag in clubID is wrong? player uid = %u, amount = %u, clubID = %u", pPlayer->nUserUID, nAmount, nClubID);
				return true;
			}
			else if (nClubID != isClubRoom()) {
				if (((ThirteenRoom*)m_pRoom)->isLeagueRoom() == 0) {
					jsRet["ret"] = 6;
					sendMsgToPlayer(jsRet, nMsgType, nSessionID);
					LOGFMTE("Drag in clubID is wrong? player uid = %u, amount = %u, clubID = %u", pPlayer->nUserUID, nAmount, nClubID);
					return true;
				}
			}
		}
		
		Json::Value jsReq;
		jsReq["playerUID"] = pPlayer->nUserUID;
		jsReq["roomID"] = getRoomID();
		jsReq["clubID"] = nClubID;
		jsReq["amount"] = nAmount;
		jsReq["leagueID"] = getLeagueID();
		jsReq["roomName"] = getOpts()["name"];
		jsReq["roomLevel"] = getOpts()["level"];
		jsReq["mtt"] = 1;
		jsReq["initialCoin"] = getInitialCoin();
		jsReq["dragIn"] = pPlayer->isDragIn ? 1 : 0;
		jsReq["needVerify"] = pPlayer->isDragIn ? 1 : (m_bNeedVerify ? 1 : 0);
		auto pApp = m_pRoomMgr->getSvrApp();
		pApp->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, pPlayer->nUserUID, eAsync_player_apply_DragIn, jsReq, [pApp, this, pPlayer, nSessionID, nAmount, nClubID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
		{
			Json::Value jsRet;
			if (isTimeOut)
			{
				LOGFMTE(" request of baseData apply drag in time out uid = %u , can not drag in ", pPlayer->nUserUID);
				jsRet["ret"] = 7;
				sendMsgToPlayer(jsRet, MSG_ROOM_THIRTEEN_APPLAY_DRAG_IN, nSessionID);
				return;
			}

			uint8_t nReqRet = retContent["ret"].asUInt();
			uint8_t nRet = 0;
			do {
				if (0 != nReqRet)
				{
					nRet = (nReqRet == 8) ? 8 : 4;
					break;
				}

				pPlayer->nClubID = nClubID;
				pPlayer->bWaitDragIn = true;

			} while (0);

			jsRet["ret"] = nRet;
			sendMsgToPlayer(jsRet, MSG_ROOM_THIRTEEN_APPLAY_DRAG_IN, nSessionID);
		});
	}
	break;
	case MSG_ROOM_THIRTEEN_NEED_DRAGIN:
	{
		/*if (isClubRoom() == 0) {
			Json::Value jsRet;
			jsRet["ret"] = 1;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("current room do not need to drag in? session id = %u", nSessionID);
			return true;
		}*/

		auto pPlayer = (stwStayPlayer*)isEnterBySession(nSessionID);
		if (pPlayer == nullptr)
		{
			uint32_t nUserID = prealMsg["uid"].isUInt() ? prealMsg["uid"].asUInt() : 0;
			if (nUserID == 0) {
				Json::Value jsRet;
				jsRet["ret"] = 2;
				sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("you are not in this room how to apply drag in? session id = %u", nSessionID);
				return true;
			}
			pPlayer = (stwStayPlayer*)isEnterByUserID(nUserID);
			if (pPlayer == nullptr) {
				pPlayer = new stwStayPlayer();
				pPlayer->nSessionID = 0;
				pPlayer->nChip = 0;
				pPlayer->nUserUID = nUserID;
				pPlayer->nEnterClubID = prealMsg["enterClubID"].asUInt();
				m_mStayPlayers[pPlayer->nUserUID] = pPlayer;
			}
		}
		auto tRet = canPlayerDragIn(pPlayer->nUserUID);
		if (tRet) {
			Json::Value jsRet;
			jsRet["ret"] = tRet;
			sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("apply drag in list error with ret = %u? user id = %u", tRet, pPlayer->nUserUID);
			return true;
		}

		auto pRoom = (ThirteenRoom*)getCoreRoom();
		Json::Value jsMsg;
		jsMsg["ret"] = 0;
		jsMsg["min"] = pRoom->getMinDragIn();
		jsMsg["max"] = pRoom->getMaxDragIn();
		jsMsg["enterClubID"] = pPlayer->nEnterClubID;
		jsMsg["dragIn"] = pPlayer->isDragIn ? 1 : 0;
		if (pPlayer->isDragIn) {
			Json::Value jsClubs;
			jsClubs[jsClubs.size()] = pPlayer->nClubID;
			jsMsg["clubIDs"] = jsClubs;
			sendMsgToPlayer(jsMsg, MSG_ROOM_THIRTEEN_NEED_DRAGIN, nSessionID);
		}
		else {
			if (getLeagueID()) {
				Json::Value jsReq;
				jsReq["playerUID"] = pPlayer->nUserUID;
				jsReq["leagueID"] = getLeagueID();
				m_pRoomMgr->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, pPlayer->nUserUID, eAsync_player_apply_DragIn_Clubs, jsReq, [this, pPlayer, nSessionID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
				{
					if (isTimeOut || pPlayer == nullptr)
					{
						LOGFMTE("inform player drag in clubs error,time out  room id = %u", getRoomID());
						Json::Value jsClubs;
						jsClubs[jsClubs.size()] = isClubRoom();
						jsUserData["clubIDs"] = jsClubs;
						sendMsgToPlayer(jsUserData, MSG_ROOM_THIRTEEN_NEED_DRAGIN, nSessionID);
						return;
					}

					if (retContent["ret"].asUInt() != 0)
					{
						LOGFMTE("inform player drag in clubs error, request error, room id = %u , uid = %u", getRoomID(), pPlayer->nUserUID);
						Json::Value jsClubs;
						jsClubs[jsClubs.size()] = isClubRoom();
						jsUserData["clubIDs"] = jsClubs;
						sendMsgToPlayer(jsUserData, MSG_ROOM_THIRTEEN_NEED_DRAGIN, nSessionID);
						return;
					}
					//Json::Value jsClubs;
					//jsClubs[jsClubs.size()] = isClubRoom();
					jsUserData["clubIDs"] = retContent["clubIDs"];
					sendMsgToPlayer(jsUserData, MSG_ROOM_THIRTEEN_NEED_DRAGIN, nSessionID);
				}, jsMsg);
			}
			else {
				Json::Value jsClubs;
				jsClubs[jsClubs.size()] = isClubRoom();
				jsMsg["clubIDs"] = jsClubs;
				sendMsgToPlayer(jsMsg, MSG_ROOM_THIRTEEN_NEED_DRAGIN, nSessionID);
			}
		}
	}
	break;
	default:
	{
		if (setCoreRoomBySessionID(nSessionID)) {
			return IPrivateRoom::onMsg(prealMsg, nMsgType, eSenderPort, nSessionID);
		}
		return false;
	}
	}
	return true;
}

bool ThirteenWPrivateRoom::onPlayerDragIn(uint32_t nUserID, uint32_t nClubID, uint32_t nAmount) {
	nAmount = getInitialCoin();
	if (nAmount == 0) {
		return false;
	}
	auto stw = (stwStayPlayer*)isEnterByUserID(nUserID);
	if (nullptr == stw) {
		return false;
	}
	if (stw->nClubID && stw->nClubID != nClubID) {
		return false;
	}

	if (stw->isDragIn) {
		if (m_tMTTBlindRise.isRunning() && m_nCurBlind > m_nRebuyLevel) {
			return false;
		}

		if (m_nRebuyTime && stw->nRebuyTime >= m_nRebuyTime) {
			return false;
		}
	}
	else {
		if (getPlayerCnt() >= getMaxCnt()) {
			return false;
		}
	}
	
	if (setCoreRoomByUserID(nUserID) && ((ThirteenRoom*)getCoreRoom())->onPlayerDragIn(nUserID, nAmount)) {
		stw->nChip = getCoreRoom()->getPlayerByUID(nUserID)->getChips();
	}
	else {
		stw->nChip += nAmount;
		if (stw->nChip < ((ThirteenRoom*)getCoreRoom())->getDragInNeed()) {
			stw->nChip = ((ThirteenRoom*)getCoreRoom())->getDragInNeed();
		}
	}
	if (stw->isDragIn) {
		stw->nRebuyTime += 1;
	}
	stw->bWaitDragIn = false;
	stw->isDragIn = true;
	stw->nAllWrag += nAmount;
	stw->tOutTime = 0;
	stw->nOutGIdx = 0;
	//st->nClubID = nClubID;
	getRoomRecorder()->addDragIn(nUserID, nAmount, stw->nClubID);
	return true;
}

bool ThirteenWPrivateRoom::onPlayerDeclineDragIn(uint32_t nUserID) {
	if (m_mStayPlayers.count(nUserID) == 0) {
		return false;
	}
	auto stg = (stwStayPlayer*)m_mStayPlayers[nUserID];
	if (setCoreRoomByUserID(nUserID)) {
		return ThirteenPrivateRoom::onPlayerDeclineDragIn(nUserID);
	}
	else {
		stg->bWaitDragIn = false;
		if (stg->isDragIn == false) {
			stg->nClubID = 0;
		}
		return true;
	}
	return true;
}

uint8_t ThirteenWPrivateRoom::canPlayerDragIn(uint32_t nUserUID) {
	if (isClubRoom() == 0) {
		return 10;
	}

	/*if (m_tMTTBlindRise.isRunning() && m_nCurBlind > m_nRebuyLevel) {
		return 11;
	}*/

	auto sPlayer = (stwStayPlayer*)isEnterByUserID(nUserUID);
	if (sPlayer == nullptr || sPlayer->bWaitDragIn)
	{
		return 1;
	}

	if (sPlayer->isDragIn) {
		if (sPlayer->nChip >= ((ThirteenRoom*)getCoreRoom())->getDragInNeed()) {
			return 14;
		}
		//tobe 20180518
		if (m_tMTTBlindRise.isRunning() && m_nCurBlind > m_nRebuyLevel) {
			return 11;
		}
	}
	if (sPlayer->isDragIn == false && m_tMTTBlindRise.isRunning() && m_nCurBlind > m_nDelayEnterLevel) {
		return 12;
	}
	if (m_nRebuyTime && sPlayer->isDragIn && sPlayer->nRebuyTime >= m_nRebuyTime) {
		return 13;
	}
	return 0;
}

void ThirteenWPrivateRoom::doRoomGameOver(bool isDismissed) {
	if (eState_RoomOvered == m_nPrivateRoomState)  // avoid opotion  loop invoke this function ;
	{
		LOGFMTE("already gave over , why invoker again room id = %u", getRoomID());
		return;
	}

	getRoomRecorder()->setPlayerCnt(getPlayerCnt());
	getRoomRecorder()->setRotBankerPool(m_nRotBankerPool);
	getRoomRecorder()->setDuration(time(NULL) - m_nStartTime);

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

bool ThirteenWPrivateRoom::doDeleteRoom() {
	getRoomRecorder()->doSaveRoomRecorder(m_pRoomMgr->getSvrApp()->getAsynReqQueue());
	m_tWaitReplyDismissTimer.canncel();
	m_tAutoDismissTimer.canncel();

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

	for (auto& ref : m_vPRooms) {
		Json::Value jsMsg;
		ref->sendRoomMsg(jsMsg, MSG_ROOM_PLAYER_LEAVE);
		ref->doDeleteRoom();
	}

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

bool ThirteenWPrivateRoom::isRoomFull() {
	return false;
}

void ThirteenWPrivateRoom::onDismiss() {
	m_nLeftRounds = 0;
}

uint32_t ThirteenWPrivateRoom::getBlindBaseScore() {
	if (m_mMTTLevelInfo.count(m_nCurBlind)) {
		return m_mMTTLevelInfo[m_nCurBlind].nBaseScore;
	}
	return m_mMTTLevelInfo.rbegin()->second.nBaseScore;
}

uint32_t ThirteenWPrivateRoom::getBlindPreScore() {
	if (m_mMTTLevelInfo.count(m_nCurBlind)) {
		return m_mMTTLevelInfo[m_nCurBlind].nPreScore;
	}
	return m_mMTTLevelInfo.rbegin()->second.nPreScore;
}

void ThirteenWPrivateRoom::onPlayerRotBanker(IGamePlayer* pPlayer, uint8_t nCoin) {
	m_nRotBankerPool += nCoin;
	getRoomRecorder()->addRotBankerPool(pPlayer->getUserUID(), nCoin);
}

void ThirteenWPrivateRoom::update(float fDelta) {
	for (auto& ref : m_vPRooms) {
		if (ref) {
			ref->update(fDelta);
		}
	}

	if (isRoomGameOver()) {
		for (auto& ref : m_vPRooms) {
			if (ref) {
				if (((ThirteenRoom*)ref)->isRoomWaiting()) {
					continue;
				}
				else {
					if (ref->getCurState()->getStateID() == eRoomSate_WaitReady && ref->canStartGame() == false) {
						((ThirteenRoom*)ref)->clearRoom();
						((ThirteenRoom*)ref)->signIsWaiting();
						continue;
					}
					return;
				}
			}
		}
		doRoomGameOver(false);
		return;
	}

	if (m_tMTTBlindRise.isRunning() == false) {
		auto tNow = time(NULL);
		if (tNow > m_nStartTime) {
			onRoomStart();
		}
		else {
			return;
		}
	}
	else {
		if (getPlayerCnt() == 0 && m_nCurBlind > m_nRebuyLevel && m_nCurBlind > m_nDelayEnterLevel) {
			onDismiss();
			return;
		}
	}

	std::vector<stwStayPlayer*> vWait;
	for (auto ref : m_mStayPlayers) {
		auto stg = (stwStayPlayer*)ref.second;
		if (stg->bLeaved && stg->isDragIn && stg->nChip > 0) {
			vWait.push_back(stg);
		}
	}

	if (vWait.size()) {
		//先找空位派发
		dispatcherToEmptyPlace(vWait);

		if (vWait.size() > 1) {
			dispatcherPlayers(vWait);
		}

		if (vWait.size() < 2) {
			if (getAliveCnt() < 2 && m_nCurBlind > m_nRebuyLevel && m_nCurBlind > m_nDelayEnterLevel) {
				onDismiss();
			}
			else {
				if (vWait.size() == 1) {
					m_bNeedSplitRoom = true;
				}
				else {
					m_bNeedSplitRoom = false;
				}
			}
		}

		/*if (vWait.size() < 2 && getAliveCnt() < 2 && m_nCurBlind > m_nRebuyLevel && m_nCurBlind > m_nDelayEnterLevel) {
			onDismiss();
		}*/
	}
}

bool ThirteenWPrivateRoom::onPlayerNetStateRefreshed(uint32_t nPlayerID, eNetState nState) {
	auto stg = (stwStayPlayer*)isEnterByUserID(nPlayerID);
	if (stg) {
		/*if (nState != eNet_Offline && isRoomFull() && stg->nState == eNet_Offline && stg->bLeaved) {
			return false;
		}*/
		stg->nState = nState;
		if (nState == eNet_Offline) {
			if (setCoreRoomByUserID(nPlayerID)) {
				onPlayerDoLeaved(getCoreRoom(), stg->nUserUID);
			}
		}

		if (stg->nCurInIdx != (uint16_t)-1 && stg->nCurInIdx < m_vPRooms.size()) {
			m_pRoom = m_vPRooms[stg->nCurInIdx];
			return ThirteenPrivateRoom::onPlayerNetStateRefreshed(nPlayerID, nState);
		}
		return true;
	}
	return false;
}

bool ThirteenWPrivateRoom::onPlayerSetNewSessionID(uint32_t nPlayerID, uint32_t nSessinID) {
	auto stg = (stwStayPlayer*)isEnterByUserID(nPlayerID);
	if (stg) {
		stg->nSessionID = nSessinID;
		stg->nState = eNet_Online;
		if (stg->nCurInIdx != (uint16_t)-1 && stg->nCurInIdx < m_vPRooms.size()) {
			m_pRoom = m_vPRooms[stg->nCurInIdx];
			return ThirteenPrivateRoom::onPlayerSetNewSessionID(nPlayerID, nSessinID);
		}
		return true;
	}
	return false;
}

uint32_t ThirteenWPrivateRoom::getRoomPlayerCnt() {
	return getPlayerCnt();
}

uint16_t ThirteenWPrivateRoom::getPlayerCnt() {
	uint16_t nCnt = 0;
	for (auto ref : m_mStayPlayers) {
		if (((stwStayPlayer*)ref.second)->isDragIn) {
			nCnt++;
		}
	}
	return nCnt;
}

bool ThirteenWPrivateRoom::enterRoomToWatch(stEnterRoomData* pEnterRoomPlayer) {
	auto sPlayer = (stwStayPlayer*)isEnterByUserID(pEnterRoomPlayer->nUserUID);
	if (sPlayer) {
		if (0 == sPlayer->nSessionID) {
			return false;
		}
	}
	else {
		sPlayer = new stwStayPlayer();
		sPlayer->nSessionID = pEnterRoomPlayer->nSessionID;
		sPlayer->nChip = 0;
		sPlayer->nUserUID = pEnterRoomPlayer->nUserUID;
		sPlayer->nEnterClubID = pEnterRoomPlayer->nClubID;
		m_mStayPlayers[sPlayer->nUserUID] = sPlayer;
	}

	for (uint32_t i = 0; i < m_vPRooms.size(); i++) {
		auto ref = (ThirteenRoom*)m_vPRooms[i];
		if (ref->isRoomWaiting()) {
			continue;
		}
		ref->onPlayerEnter(pEnterRoomPlayer);
		m_pRoom = ref;
		return true;
	}
	if (m_vPRooms.size()) {
		m_vPRooms[0]->onPlayerEnter(pEnterRoomPlayer);
		m_pRoom = m_vPRooms[0];
		return true;
	}
	return false;
}

bool ThirteenWPrivateRoom::setCoreRoomBySessionID(uint32_t nSessionID) {
	for (auto ref : m_mStayPlayers) {
		auto refs = (stwStayPlayer*)ref.second;
		if (refs->nSessionID == nSessionID) {
			if (refs->nCurInIdx != (uint16_t)-1 && refs->nCurInIdx < m_vPRooms.size()) {
				m_pRoom = m_vPRooms[refs->nCurInIdx];
				return true;
			}
		}
	}
	return false;
}

bool ThirteenWPrivateRoom::setCoreRoomByUserID(uint32_t nUserID) {
	if (m_mStayPlayers.count(nUserID)) {
		auto nIdx = ((stwStayPlayer*)m_mStayPlayers[nUserID])->nCurInIdx;
		if (nIdx != uint16_t(-1) && nIdx < m_vPRooms.size()) {
			m_pRoom = m_vPRooms[nIdx];
			return true;
		}
	}
	return false;
}

bool ThirteenWPrivateRoom::dispatcherPlayers(std::vector<stwStayPlayer*>& vWait) {
	auto funDispatcher = [this](std::vector<stwStayPlayer*>& vTemp) {
		auto pRoom = findWaitingRoom();
		if (pRoom == nullptr) {
			pRoom = doCreatRealRoom();
			if (!pRoom)
			{
				LOGFMTE("create private room error ");
				return false;
			}
			LOGFMTD("create 1 private room");
			auto bRet = pRoom->init(m_pRoomMgr, getSeiralNum(), getRoomID(), m_pRoom->getSeatCnt(), getOpts());
			if (!bRet)
			{
				LOGFMTE("init private room error ");
				return false;
			}
			((ThirteenRoom*)pRoom)->signIsWaiting();
			pRoom->setDelegate(this);
			m_vPRooms.push_back(pRoom);
		}
		m_pRoom = pRoom;
		uint8_t nSeatIdx = 0;
		for (auto& ref : vTemp) {
			stEnterRoomData strd;
			strd.nChip = ref->nChip;
			strd.nSessionID = ref->nSessionID;
			strd.nUserUID = ref->nUserUID;
			if (pRoom->onPlayerEnter(&strd) && pRoom->doPlayerSitDown(&strd, nSeatIdx)) {
				sendRoomInfo(strd.nSessionID);
				ref->isSitdown = true;
			}
			else {
				LOGFMTE("big error, player can not enter!!! roomID = %u", getRoomID());
				return false;
			}
			nSeatIdx++;
		}
		((ThirteenRoom*)m_pRoom)->clearIsWaiting();
		return true;
	};

	if (m_pRoom->getSeatCnt() == 0) {
		return false;
	}

	while (vWait.size() >= m_pRoom->getSeatCnt() * 2) {
		std::vector<stwStayPlayer*> vTemp;
		for (uint8_t i = 0; i < m_pRoom->getSeatCnt(); i++) {
			uint16_t randIdx = rand() % vWait.size();
			vTemp.push_back(vWait[randIdx]);
			vWait.erase(vWait.begin() + randIdx);
		}
		if (funDispatcher(vTemp) == false) {
			return false;
		}
	}

	if (vWait.size() && vWait.size() <= m_pRoom->getSeatCnt()) {
		if (funDispatcher(vWait) == false) {
			return false;
		}
	}
	else if (vWait.size() > m_pRoom->getSeatCnt()) {
		uint8_t nNeedCnt = vWait.size() / 2;
		std::vector<stwStayPlayer*> vTemp;
		for (uint8_t i = 0; i < nNeedCnt; i++) {
			uint16_t randIdx = rand() % vWait.size();
			vTemp.push_back(vWait[randIdx]);
			vWait.erase(vWait.begin() + randIdx);
		}
		if (funDispatcher(vTemp) == false) {
			return false;
		}
		if (funDispatcher(vWait) == false) {
			return false;
		}
	}
	return true;
}

void ThirteenWPrivateRoom::dispatcherToEmptyPlace(std::vector<stwStayPlayer*>& vWait) {
	if (vWait.size() == 0) {
		return;
	}
	uint32_t nEmptyCnt = 0;
	for (auto ref : m_vPRooms) {
		if (((ThirteenRoom*)ref)->isRoomWaiting()) {
			continue;
		}
		uint8_t nCnt = ref->getSeatCnt();
		for (uint8_t i = 0; i < nCnt; i++) {
			if (ref->getPlayerByIdx(i)) {
				continue;
			}
			if (vWait.size() > 2 || vWait.size() == 1) {
				stEnterRoomData strd;
				strd.nChip = vWait[0]->nChip;
				strd.nSessionID = vWait[0]->nSessionID;
				strd.nUserUID = vWait[0]->nUserUID;
				if (ref->onPlayerEnter(&strd) && ref->doPlayerSitDown(&strd, i)) {
					sendRoomInfo(strd.nSessionID);
					vWait[0]->isSitdown = true;
					vWait.erase(vWait.begin());
				}
				else {
					LOGFMTE("big error, player can not enter!!! roomID = %u", getRoomID());
					return;
				}
			}
			else {
				nEmptyCnt++;
			}
		}
	}

	if (vWait.size() > nEmptyCnt || vWait.size() == 0) {
		return;
	}

	for (auto ref : m_vPRooms) {
		if (((ThirteenRoom*)ref)->isRoomWaiting()) {
			continue;
		}
		uint8_t nCnt = ref->getSeatCnt();
		for (uint8_t i = 0; i < nCnt; i++) {
			if (ref->getPlayerByIdx(i)) {
				continue;
			}
			stEnterRoomData strd;
			strd.nChip = vWait[0]->nChip;
			strd.nSessionID = vWait[0]->nSessionID;
			strd.nUserUID = vWait[0]->nUserUID;
			if (ref->onPlayerEnter(&strd) && ref->doPlayerSitDown(&strd, i)) {
				sendRoomInfo(strd.nSessionID);
				vWait[0]->isSitdown = true;
				vWait.erase(vWait.begin());
			}
			else {
				LOGFMTE("big error, player can not enter!!! roomID = %u", getRoomID());
				return;
			}
		}
	}
}

GameRoom* ThirteenWPrivateRoom::findWaitingRoom() {
	for (auto& ref : m_vPRooms) {
		if (((ThirteenRoom*)ref)->isRoomWaiting()) {
			return ref;
		}
	}
	return nullptr;
}

uint32_t ThirteenWPrivateRoom::getAliveCnt() {
	uint32_t nCnt = 0;
	for (auto ref : m_mStayPlayers) {
		if (ref.second->nChip > 0) {
			nCnt++;
		}
	}
	return nCnt;
}

uint32_t ThirteenWPrivateRoom::getEmptySeatCnt(IGameRoom* pRoom) {
	uint32_t nEmptyCnt = 0;
	for (auto ref : m_vPRooms) {
		if (((ThirteenRoom*)ref)->isRoomWaiting() || ref == pRoom) {
			continue;
		}
		uint8_t nCnt = ref->getSeatCnt();
		for (uint8_t i = 0; i < nCnt; i++) {
			if (ref->getPlayerByIdx(i)) {
				continue;
			}
			nEmptyCnt++;
		}
	}
	return nEmptyCnt;
}

void ThirteenWPrivateRoom::initLevelInfo() {
	mttLevelInfo mli;
	uint8_t i = 0;
	mli.nLevel = 1;
	mli.nBaseScore = 10;
	mli.nPreScore = 0;
	m_mMTTLevelInfo[mli.nLevel] = mli;

	mli.nLevel = 2;
	mli.nBaseScore = 15;
	mli.nPreScore = 0;
	m_mMTTLevelInfo[mli.nLevel] = mli;

	mli.nLevel = 3;
	mli.nBaseScore = 25;
	mli.nPreScore = 0;
	m_mMTTLevelInfo[mli.nLevel] = mli;

	//4 - 6
	for (i = 0; i < 3; i++) {
		mli.nLevel = 4 + i;
		mli.nBaseScore = 50 + i * 25;
		mli.nPreScore = mli.nLevel == 6 ? 10 : 0;
		m_mMTTLevelInfo[mli.nLevel] = mli;
	}

	//7 - 8
	mli.nLevel = 7;
	mli.nBaseScore = 150;
	mli.nPreScore = 15;
	m_mMTTLevelInfo[mli.nLevel] = mli;

	mli.nLevel = 8;
	mli.nBaseScore = 200;
	mli.nPreScore = 20;
	m_mMTTLevelInfo[mli.nLevel] = mli;

	//9 - 10
	mli.nLevel = 9;
	mli.nBaseScore = 300;
	mli.nPreScore = 30;
	m_mMTTLevelInfo[mli.nLevel] = mli;

	mli.nLevel = 10;
	mli.nBaseScore = 400;
	mli.nPreScore = 40;
	m_mMTTLevelInfo[mli.nLevel] = mli;

	//11 - 13
	for (i = 0; i < 3; i++) {
		mli.nLevel = 11 + i;
		mli.nBaseScore = 600 + i * 200;
		mli.nPreScore = mli.nBaseScore * 3 / 10;
		m_mMTTLevelInfo[mli.nLevel] = mli;
	}

	//14 - 15
	mli.nLevel = 14;
	mli.nBaseScore = 1500;
	mli.nPreScore = 450;
	m_mMTTLevelInfo[mli.nLevel] = mli;

	mli.nLevel = 15;
	mli.nBaseScore = 2000;
	mli.nPreScore = 600;
	m_mMTTLevelInfo[mli.nLevel] = mli;

	//16 - 17
	mli.nLevel = 16;
	mli.nBaseScore = 3000;
	mli.nPreScore = 1500;
	m_mMTTLevelInfo[mli.nLevel] = mli;

	mli.nLevel = 17;
	mli.nBaseScore = 4000;
	mli.nPreScore = 2000;
	m_mMTTLevelInfo[mli.nLevel] = mli;

	//18 - 20
	for (i = 0; i < 3; i++) {
		mli.nLevel = 18 + i;
		mli.nBaseScore = 6000 + i * 2000;
		mli.nPreScore = mli.nBaseScore * 5 / 10;
		m_mMTTLevelInfo[mli.nLevel] = mli;
	}

	//21 - 28
	for (i = 0; i < 8; i++) {
		mli.nLevel = 21 + i;
		mli.nBaseScore = 15000 + i * 5000;
		mli.nPreScore = mli.nBaseScore;
		m_mMTTLevelInfo[mli.nLevel] = mli;
	}

	//29 - 47
	for (i = 0; i < 19; i++) {
		mli.nLevel = 29 + i;
		mli.nBaseScore = 60000 + i * 10000;
		mli.nPreScore = mli.nBaseScore;
		m_mMTTLevelInfo[mli.nLevel] = mli;
	}

	//48 - 50
	for (i = 0; i < 3; i++) {
		mli.nLevel = 48 + i;
		mli.nBaseScore = 260000 + i * 20000;
		mli.nPreScore = mli.nBaseScore;
		m_mMTTLevelInfo[mli.nLevel] = mli;
	}
}

void ThirteenWPrivateRoom::sendBssicRoomInfo(uint32_t nSessionID, uint32_t nUserID) {
	Json::Value jsMsg;
	bool isOpen = m_tMTTBlindRise.isRunning();
	jsMsg["ret"] = 0;
	jsMsg["roomID"] = getRoomID();
	jsMsg["curBlind"] = m_nCurBlind;
	jsMsg["riseTime"] = (int32_t)m_tMTTBlindRise.getDuringTime();
	jsMsg["dragInCnt"] = getPlayerCnt();
	jsMsg["aliveCnt"] = getAliveCnt();
	jsMsg["isOpen"] = isOpen ? 1 : 0;
	uint32_t nAllChips = 0;
	uint32_t nRebuyNumber = 0;
	for (auto ref : m_mStayPlayers) {
		nAllChips += ref.second->nChip;
		if (((stwStayPlayer*)(ref.second))->nRebuyTime) {
			nRebuyNumber++;
		}
	}
	jsMsg["allChips"] = nAllChips;
	jsMsg["rebuyCnt"] = nRebuyNumber;

	auto sPlayer = (stwStayPlayer*)isEnterByUserID(nUserID);
	jsMsg["waitDragIn"] = sPlayer && sPlayer->bWaitDragIn ? 1 : 0;
	if (sPlayer && sPlayer->isDragIn) {
		if (isOpen && sPlayer->nOutGIdx) {
			jsMsg["state"] = MTT_PLAYER_TOUT;
		}
		else {
			jsMsg["state"] = MTT_PLAYER_DRAGIN;
		}
	}
	else {
		jsMsg["state"] = MTT_PLAYER_NOT_DRAGIN;
	}

	jsMsg["opts"] = getOpts();
	sendMsgToPlayer(jsMsg, MSG_ROOM_REQUEST_THIRTEEN_ROOM_INFO, nSessionID);
}

void ThirteenWPrivateRoom::onRoomStart() {
	m_tMTTBlindRise.start();
	Json::Value jsMsg;
	jsMsg["roomID"] = getRoomID();
	jsMsg["port"] = ID_MSG_PORT_THIRTEEN;
	pushRoomMsgToAllPlayer(jsMsg, MSG_ROOM_THIRTEEN_MTT_GAME_START);
}

void ThirteenWPrivateRoom::sendRoomMsgToAllPlayer(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nOmitSessionID) {
	for (auto ref : m_mStayPlayers) {
		if (ref.second->nSessionID) {
			if (nOmitSessionID && ref.second->nSessionID == nOmitSessionID) {
				continue;
			}
			sendMsgToPlayer(prealMsg, nMsgType, ref.second->nSessionID);
		}
	}
}

void ThirteenWPrivateRoom::pushRoomMsgToAllPlayer(Json::Value& prealMsg, uint16_t nMsgType, bool isDragIn) {
	auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
	for (auto ref : m_mStayPlayers) {
		if (isDragIn && ref.second->isDragIn == false) {
			continue;
		}
		if (ref.second->nSessionID) {
			sendMsgToPlayer(prealMsg, nMsgType, ref.second->nSessionID);
		}
		else {
			Json::Value jsReqInfo;
			jsReqInfo = prealMsg;
			jsReqInfo["targetUID"] = ref.second->nUserUID;
			jsReqInfo["msgType"] = nMsgType;
			pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, ref.second->nUserUID, eAsync_thirteen_MTT_Request_PushMsg, jsReqInfo);
		}
	}
}

bool ThirteenWPrivateRoom::isPlayerDragIn(uint32_t nUserID) {
	auto sPlayer = (stwStayPlayer*)isEnterByUserID(nUserID);
	if (sPlayer) {
		return sPlayer->isDragIn;
	}
	return false;
}