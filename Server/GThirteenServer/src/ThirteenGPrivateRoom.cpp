#include "ThirteenGPrivateRoom.h"
#include "Thirteen\ThirteenRoom.h"
#include "stEnterRoomData.h"
#include "IGameRoomManager.h"
#include "ISeverApp.h"
#include "IGamePlayer.h"
#include "AsyncRequestQuene.h"
#include "Thirteen\ThirteenPlayer.h"
#define MAX_WAIT_TIME 5
#define AUTO_PICK_OUT_TIME 180

ThirteenGPrivateRoom::~ThirteenGPrivateRoom() {
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

bool ThirteenGPrivateRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts) {
	if (ThirteenPrivateRoom::init(pRoomMgr, nSeialNum, nRoomID, nSeatCnt, vJsOpts)) {
		m_vPRooms.push_back(m_pRoom);
		initMaxPlayerCnt();
		return true;
	}
	return false;
}

void ThirteenGPrivateRoom::setCurrentPointer(IGameRoom* pRoom) {
	if (std::find(m_vPRooms.begin(), m_vPRooms.end(), pRoom) == m_vPRooms.end()) {
		//assert(0 && "invalid argument");
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

bool ThirteenGPrivateRoom::onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)
{
	pEnterRoomPlayer->nChip = 0;
	if (m_vPRooms.empty() || m_pRoom == nullptr)
	{
		LOGFMTE("why room is null ? ");
		return false;
	}

	/*if (isRoomFull()) {
		Json::Value
		sendMsgToPlayer();
		return false;
	}*/
	//bool bNeedDragIn = false;
	/*if (m_mStayPlayers.count(pEnterRoomPlayer->nUserUID)) {
		
	}*/

	if (m_mStayPlayers.count(pEnterRoomPlayer->nUserUID)) {
		auto pStay = (stgStayPlayer*)m_mStayPlayers[pEnterRoomPlayer->nUserUID];
		pStay->nSessionID = pEnterRoomPlayer->nSessionID;
		pEnterRoomPlayer->nChip = pStay->nChip;
		pStay->nState = eNet_Online;

		uint16_t nPointerIdx = pStay->nCurInIdx;
		if (nPointerIdx == (uint16_t)-1) {
			packTempRoomInfoToPlayer(pEnterRoomPlayer);
			return false;
		}
		else {
			if (nPointerIdx < m_vPRooms.size()) {
				m_pRoom = m_vPRooms[nPointerIdx];
				if (m_pRoom->onPlayerEnter(pEnterRoomPlayer)) {
					
				}
				return true;
			}
			else {
				pStay->nCurInIdx = -1;
				packTempRoomInfoToPlayer(pEnterRoomPlayer);
				return false;
			}
		}
	}
	else {
		stgStayPlayer* pStay = new stgStayPlayer();
		pStay->nSessionID = pEnterRoomPlayer->nSessionID;
		pStay->nChip = pEnterRoomPlayer->nChip;
		/*pStay->nCurInIdx = -1;
		pStay->nState = eNet_Online;*/
		pStay->nUserUID = pEnterRoomPlayer->nUserUID;
		/*pStay->bNeedDragIn = true;
		pStay->bLeaved = true;
		pStay->bAutoStandup = false;
		pStay->bAutoLeave = false;*/
		m_mStayPlayers[pStay->nUserUID] = pStay;
		packTempRoomInfoToPlayer(pEnterRoomPlayer);
		return false;
	}

	/*if (m_pRoom->onPlayerEnter(pEnterRoomPlayer))
	{

	}*/
	//TODO
	/*for (auto& ref : m_vPRooms) {
		if (ref->isRoomFull()) {
			continue;
		}
		if (ref->onPlayerEnter(pEnterRoomPlayer)) {
			return true;
		}
	}*/

	return true;
}

bool ThirteenGPrivateRoom::initMaxPlayerCnt() {
	uint8_t nAmountLevel = m_nRoundLevel >> 4;
	if (nAmountLevel > 5) {
		LOGFMTE("invalid room level , level = %u", nAmountLevel);
		nAmountLevel = 5;
	}
	uint16_t vAmount[] = { 4, 20, 30, 50, 100, 200 };
	
	m_nMaxCnt = vAmount[nAmountLevel];
	return true;
}

bool ThirteenGPrivateRoom::packTempRoomInfoToPlayer(stEnterRoomData* pEnterRoomPlayer) {
	ThirteenRoom tempRoom;
	if (tempRoom.init(m_pRoomMgr, getSeiralNum(), getRoomID(), m_pRoom->getSeatCnt(), getOpts())) {
		tempRoom.setDelegate(this);
		//if (tempRoom.onPlayerEnter(pEnterRoomPlayer) && tempRoom.doPlayerSitDown(pEnterRoomPlayer, 0)) {
		//	auto pRoomTemp = m_pRoom;
		//	m_pRoom = &tempRoom;
		//	sendRoomInfo(pEnterRoomPlayer->nSessionID);
		//	m_pRoom = pRoomTemp;
		//	//tempRoom.clearRoom();
		//	return true;
		//}
		if (tempRoom.onPlayerEnter(pEnterRoomPlayer)) {
			auto pRoomTemp = m_pRoom;
			m_pRoom = &tempRoom;
			sendRoomInfo(pEnterRoomPlayer->nSessionID);
			//tempRoom.doPlayerSitDown(pEnterRoomPlayer, 0);
			m_pRoom = pRoomTemp;
			return true;
		}
	}
	return false;
}

bool ThirteenGPrivateRoom::setCoreRoomBySessionID(uint32_t nSessionID) {
	for (auto ref : m_mStayPlayers) {
		auto refs = (stgStayPlayer*)ref.second;
		if (refs->nSessionID == nSessionID) {
			if (refs->nCurInIdx != (uint16_t)-1 && refs->nCurInIdx < m_vPRooms.size()) {
				m_pRoom = m_vPRooms[refs->nCurInIdx];
				return true;
			}
		}
	}
	return false;
}

bool ThirteenGPrivateRoom::setCoreRoomByUserID(uint32_t nUserID) {
	if (m_mStayPlayers.count(nUserID)) {
		auto nIdx = ((stgStayPlayer*)m_mStayPlayers[nUserID])->nCurInIdx;
		if (nIdx != uint16_t(-1) && nIdx < m_vPRooms.size()) {
			m_pRoom = m_vPRooms[nIdx];
			return true;
		}
	}
	return false;
}

void ThirteenGPrivateRoom::onPlayerWaitDragIn(uint32_t nUserUID) {
	if (m_mStayPlayers.count(nUserUID)) {
		((stgStayPlayer*)m_mStayPlayers[nUserUID])->bNeedDragIn = true;
	}
}

uint8_t ThirteenGPrivateRoom::checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer) {
	if (isAAPay()) {
		return 8;
	}

	if (isWinerPay()) {
		return 9;
	}

	//if (isAAPay() && pEnterRoomPlayer->nDiamond < getDiamondNeed(m_nRoundLevel, getPayType()))
	//{
	//	// diamond is not enough 
	//	return 8;
	//}

	//if (isWinerPay() && pEnterRoomPlayer->nDiamond < getDiamondNeed(m_nRoundLevel, getPayType()))
	//{
	//	return 9;
	//}

	if (isRoomFull()) {
		return 10;
	}

	/*if (std::count_if(m_mStayPlayers.begin(), m_mStayPlayers.end(), [](MAP_UID_PLAYERS::value_type ref) {
		auto sref = (stgStayPlayer*)ref.second;
		return sref->nState != eNet_Offline;
	}) >= getMaxCnt()) {
		return 10;
	}*/

	return 0;
}

void ThirteenGPrivateRoom::doPlayerEnter(IGameRoom* pRoom, uint32_t nUserUID) {
	if (m_mStayPlayers.count(nUserUID)) {
		auto stg = (stgStayPlayer*)m_mStayPlayers[nUserUID];
		for (uint16_t i = 0; i < m_vPRooms.size(); i++) {
			if (m_vPRooms[i] == pRoom) {
				stg->nCurInIdx = i;
				stg->bLeaved = false;
				return;
			}
		}
	}
}

void ThirteenGPrivateRoom::onPlayerWillSitDown(IGameRoom* pRoom, uint32_t nUserUID) {
	auto sPlayer = (stgStayPlayer*)isEnterByUserID(nUserUID);
	if (sPlayer) {
		sPlayer->bNeedDragIn = false;
	}
}

void ThirteenGPrivateRoom::onPlayerSitDown(IGameRoom* pRoom, IGamePlayer* pPlayer) {
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
}

bool ThirteenGPrivateRoom::isRoomFull() {
	return std::count_if(m_mStayPlayers.begin(), m_mStayPlayers.end(), [](MAP_UID_PLAYERS::value_type ref) {
		auto sref = (stgStayPlayer*)ref.second;
		return sref->nState != eNet_Offline || sref->bLeaved == false;
	}) >= getMaxCnt();
}

void ThirteenGPrivateRoom::onGameDidEnd(IGameRoom* pRoom) {
	m_pRoom = (ThirteenRoom*)pRoom;

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
			stg->isJoin = true;
		}
	}
	//ThirteenPrivateRoom::onGameDidEnd(pRoom);

	if (isRoomGameOver()) {
		doSendRoomGameOverInfoToClient(false);
	}

	((ThirteenRoom*)pRoom)->clearRoom();
	((ThirteenRoom*)pRoom)->signIsWaiting();
}

bool ThirteenGPrivateRoom::doDeleteRoom()
{
	m_tWaitReplyDismissTimer.canncel();
	m_tAutoDismissTimer.canncel();
	// tell client closed room ;
	Json::Value jsDoClosed;
	jsDoClosed["roomID"] = getRoomID();
	jsDoClosed["isDismiss"] = m_nApplyDismissUID > 0 ? 1 : 0;
	sendRoomMsg(jsDoClosed, MSG_VIP_ROOM_DO_CLOSED);

	// tell data svr , the room is closed 
	Json::Value jsReqInfo;
	jsReqInfo["targetUID"] = m_nOwnerUID;
	jsReqInfo["roomID"] = getRoomID();
	jsReqInfo["port"] = m_pRoomMgr->getSvrApp()->getLocalSvrMsgPortType();
	auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
	pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, m_nOwnerUID, eAsync_Inform_RoomDeleted, jsReqInfo);

	for (auto ref : m_mStayPlayers) {
		auto stg = (stgStayPlayer*)ref.second;
		stg->nState = eNet_Offline;
	}
	for (auto& ref : m_vPRooms) {
		ref->doDeleteRoom();
	}
	for (auto ref : m_mStayPlayers) {
		auto stg = (stgStayPlayer*)ref.second;
		if (stg->bLeaved) {
			continue;
		}
		onPlayerDoLeaved(nullptr, stg->nUserUID);
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

	return /*m_pRoom->doDeleteRoom()*/true;
}

void ThirteenGPrivateRoom::onPlayerDoLeaved(IGameRoom* pRoom, uint32_t nUserUID) {
	if (m_mStayPlayers.count(nUserUID)) {
		auto stg = (stgStayPlayer*)m_mStayPlayers[nUserUID];
		stg->bLeaved = true;
		stg->nCurInIdx = -1;
		if (stg->bAutoLeave) {
			if (stg->nState == eNet_Online) {
				stg->nState = eNet_Offline;
				Json::Value jsMsg;
				sendMsgToPlayer(jsMsg, MSG_ROOM_PLAYER_LEAVE, stg->nSessionID);
			}
		}
		if (stg->nState == eNet_Offline) {
			ThirteenPrivateRoom::onPlayerDoLeaved(pRoom, nUserUID);
		}
		else {
			if (stg->bAutoStandup || stg->bNeedDragIn) {
				stEnterRoomData pEnterRoomPlayer;
				pEnterRoomPlayer.nChip = stg->nChip;
				pEnterRoomPlayer.nSessionID = stg->nSessionID;
				pEnterRoomPlayer.nUserUID = stg->nUserUID;
				packTempRoomInfoToPlayer(&pEnterRoomPlayer);
			}
		}
	}
}

void ThirteenGPrivateRoom::update(float fDelta) {
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
					return;
				}
			}
		}
		doRoomGameOver(false);
		return;
	}

	std::vector<stgStayPlayer*> vWait;
	for (auto ref : m_mStayPlayers) {
		auto stg = (stgStayPlayer*)ref.second;
		if (stg->bLeaved && stg->nState != eNet_Offline && stg->bNeedDragIn == false && stg->bAutoStandup == false) {
			vWait.push_back(stg);
		}
		else if (stg->bLeaved && (stg->bNeedDragIn || stg->bAutoStandup) && stg->nState != eNet_Offline) {
			stg->bWaitDragInTime += fDelta;
			if (stg->bWaitDragInTime > AUTO_PICK_OUT_TIME) {
				if (stg->nState == eNet_Online) {
					stg->nState = eNet_Offline;
					Json::Value jsMsg;
					sendMsgToPlayer(jsMsg, MSG_ROOM_PLAYER_LEAVE, stg->nSessionID);
				}
				stg->bWaitDragInTime = 0;
				onPlayerDoLeaved(nullptr, stg->nUserUID);
				/*if (stg->nState == eNet_Online) {
					Json::Value jsMsg;
					jsMsg["idx"] = 0;
					jsMsg["uid"] = stg->nUserUID;
					sendRoomMsg(jsMsg, MSG_ROOM_STAND_UP);
				}
				ss*/
			}
		}
	}

	if (vWait.size()) {
		if (m_fWaitTime < MAX_WAIT_TIME) {
			m_fWaitTime += fDelta;
		}

		if (vWait.size() > 15) {
			dispatcherPlayers(vWait);
			m_fWaitTime = 0;
		}
		else {
			if (isRoomStarted()) {
				if (vWait.size() >= m_nAutoOpenCnt && m_fWaitTime > MAX_WAIT_TIME) {
					dispatcherPlayers(vWait);
					m_fWaitTime = 0;
				}
			}
			else {
				if (vWait.size() >= m_nStartGameCnt) {
					dispatcherPlayers(vWait);
					m_fWaitTime = 0;
				}
			}
		}
	}
	else {
		m_fWaitTime = 0;
	}
}

bool ThirteenGPrivateRoom::onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID) {
	if (setCoreRoomBySessionID(nSessionID)) {
		return ThirteenPrivateRoom::onMsg(prealMsg, nMsgType, eSenderPort, nSessionID);
	}
	else {
		switch (nMsgType) {
		case MSG_PLAYER_SIT_DOWN:
		{
			auto sPlayer = (stgStayPlayer*)isEnterBySession(nSessionID);
			if (sPlayer == nullptr) {
				Json::Value jsRet;
				jsRet["ret"] = 4;
				sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				break;
			}
			sPlayer->bNeedDragIn = false;
			//uint16_t nIdx = prealMsg["idx"].asUInt();
			Json::Value jsRoomPlayerSitDown;
			jsRoomPlayerSitDown["idx"] = 0;
			jsRoomPlayerSitDown["uid"] = sPlayer->nUserUID;
			jsRoomPlayerSitDown["isOnline"] = 1;
			jsRoomPlayerSitDown["chips"] = sPlayer->nChip;
			jsRoomPlayerSitDown["state"] = eRoomPeer_WaitNextGame;
			sendMsgToPlayer(jsRoomPlayerSitDown, MSG_ROOM_SIT_DOWN, nSessionID);
			if (isClubRoom()) {
				if (sPlayer->nChip < ((ThirteenRoom*)getCoreRoom())->getMaxLose()) {
					onPlayerWaitDragIn(sPlayer->nUserUID);
					Json::Value jsMsg;
					jsMsg["idx"] = 0;
					jsMsg["min"] = ((ThirteenRoom*)getCoreRoom())->getMinDragIn();
					jsMsg["max"] = ((ThirteenRoom*)getCoreRoom())->getMaxDragIn();
					if (getLeagueID()) {
						if (getDragInClubID(sPlayer->nUserUID)) {
							Json::Value jsClubs;
							jsClubs[jsClubs.size()] = getDragInClubID(sPlayer->nUserUID);
							jsMsg["clubIDs"] = jsClubs;
							sendMsgToPlayer(jsMsg, MSG_ROOM_THIRTEEN_NEED_DRAGIN, nSessionID);
							break;
						}
						Json::Value jsReq;
						jsReq["playerUID"] = sPlayer->nUserUID;
						jsReq["leagueID"] = getLeagueID();
						m_pRoomMgr->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, sPlayer->nUserUID, eAsync_player_apply_DragIn_Clubs, jsReq, [this, sPlayer](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
						{
							if (isTimeOut)
							{
								LOGFMTE("inform player drag in clubs error,time out  room id = %u , uid = %u", getRoomID(), sPlayer->nUserUID);
								Json::Value jsClubs;
								jsClubs[jsClubs.size()] = isClubRoom();
								jsUserData["clubIDs"] = jsClubs;
								sendMsgToPlayer(jsUserData, MSG_ROOM_THIRTEEN_NEED_DRAGIN, sPlayer->nSessionID);
								return;
							}

							if (retContent["ret"].asUInt() != 0)
							{
								LOGFMTE("inform player drag in clubs error, request error, room id = %u , uid = %u", getRoomID(), sPlayer->nUserUID);
								Json::Value jsClubs;
								jsClubs[jsClubs.size()] = isClubRoom();
								jsUserData["clubIDs"] = jsClubs;
								sendMsgToPlayer(jsUserData, MSG_ROOM_THIRTEEN_NEED_DRAGIN, sPlayer->nSessionID);
								return;
							}
							Json::Value jsClubs;
							jsClubs[jsClubs.size()] = isClubRoom();
							jsUserData["clubIDs"] = retContent["clubIDs"];
							sendMsgToPlayer(jsUserData, MSG_ROOM_THIRTEEN_NEED_DRAGIN, sPlayer->nSessionID);
						}, jsMsg);
					}
					else {
						Json::Value jsClubs;
						jsClubs[jsClubs.size()] = isClubRoom();
						jsMsg["clubIDs"] = jsClubs;
						sendMsgToPlayer(jsMsg, MSG_ROOM_THIRTEEN_NEED_DRAGIN, sPlayer->nSessionID);
					}
				}
			}
		}
		break;
		case MSG_ROOM_THIRTEEN_PLAYER_AUTO_STANDUP:
		{
			auto sPlayer = (stgStayPlayer*)isEnterBySession(nSessionID);
			if (sPlayer) {
				auto nState = prealMsg["state"].asUInt();
				if (nState) {
					sPlayer->nState = eNet_Offline;
					onPlayerDoLeaved(nullptr, sPlayer->nUserUID);
					Json::Value jsMsg;
					sendMsgToPlayer(jsMsg, MSG_ROOM_PLAYER_LEAVE, nSessionID);
				}
				else {
					sPlayer->bAutoStandup = false;
				}
			}
		}
		break;
		case MSG_ROOM_THIRTEEN_PLAYER_AUTO_LEAVE:
		{
			auto sPlayer = (stgStayPlayer*)isEnterBySession(nSessionID);
			if (sPlayer) {
				auto nState = prealMsg["state"].asUInt();
				if (nState) {
					sPlayer->nState = eNet_Offline;
					onPlayerDoLeaved(nullptr, sPlayer->nUserUID);
					Json::Value jsMsg;
					sendMsgToPlayer(jsMsg, MSG_ROOM_PLAYER_LEAVE, nSessionID);
				}
				else {
					sPlayer->bAutoLeave = false;
				}
			}
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
			jsMsg["detail"] = jsDetail;
			sendMsgToPlayer(jsMsg, nMsgType, nSessionID);
		}
		break;
		case MSG_ROOM_THIRTEEN_REAL_TIME_RECORD:
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
					if (st->isDragIn == false && st->isJoin == false) {
						continue;
					}
					cIdx++;
					Json::Value jsDetail;
					jsDetail["uid"] = st->nUserUID;
					jsDetail["chip"] = st->nChip;
					jsDetail["drag"] = st->nAllWrag;
					jsDetails[jsDetails.size()] = jsDetail;
				}
				/*for (; tIdx < tIdx + 10; tIdx++) {
					if (tIdx >= vsPlayers.size()) {
						break;
					}
					Json::Value jsDetail;
					auto st = vsPlayers.at(tIdx);
					jsDetail["uid"] = st->nUserUID;
					jsDetail["chip"] = st->nChip;
					jsDetail["drag"] = st->nAllWrag;
					jsDetails[jsDetails.size()] = jsDetail;
				}*/
				jsMsg["detail"] = jsDetails;
				pIdx++;
				sendMsgToPlayer(jsMsg, nMsgType, nSessionID);
			}
		}
		break;
		case MSG_ROOM_REQUEST_THIRTEEN_ROOM_INFO:
		{
			sendBssicRoomInfo(nSessionID);
		}
		break;
		case MSG_REQUEST_ROOM_INFO:
		{
			auto stg = (stgStayPlayer*)isEnterBySession(nSessionID);
			if (stg && stg->nState != eNet_Offline) {
				stEnterRoomData erd;
				erd.nChip = stg->nChip;
				erd.nSessionID = nSessionID;
				erd.nUserUID = stg->nUserUID;
				erd.nDiamond = 0;
				packTempRoomInfoToPlayer(&erd);
				Json::Value jsMsg;
				jsMsg["ret"] = 0;
				sendMsgToPlayer(jsMsg, MSG_REQUEST_ROOM_INFO, nSessionID);
			}
			else {
				Json::Value jsMsg;
				jsMsg["ret"] = 10;
				sendMsgToPlayer(jsMsg, nMsgType, nSessionID);
				LOGFMTD("you are not enter why apply room info? session id =%u", nSessionID);
			}
			
		}
		break;
		case MSG_ROOM_THIRTEEN_APPLAY_DRAG_IN:
		{
			Json::Value jsRet;
			if (((ThirteenRoom*)m_pRoom)->isClubRoom() == 0) {
				jsRet["ret"] = 10;
				sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("Drag in error this room can not drag in? player sessionID = %u", nSessionID);
				return true;
			}
			auto nAmount = prealMsg["amount"].asUInt();
			auto pPlayer = isEnterBySession(nSessionID);
			if (pPlayer == nullptr)
			{
				jsRet["ret"] = 1;
				sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("you are not in this room how to drag in? session id = %u", nSessionID);
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
			else if (nClubID != ((ThirteenRoom*)m_pRoom)->isClubRoom()) {
				if (((ThirteenRoom*)m_pRoom)->isLeagueRoom() == 0) {
					jsRet["ret"] = 6;
					sendMsgToPlayer(jsRet, nMsgType, nSessionID);
					LOGFMTE("Drag in clubID is wrong? player uid = %u, amount = %u, clubID = %u", pPlayer->nUserUID, nAmount, nClubID);
					return true;
				}
			}
			Json::Value jsReq;
			jsReq["playerUID"] = pPlayer->nUserUID;
			jsReq["roomID"] = getRoomID();
			jsReq["clubID"] = nClubID;
			jsReq["amount"] = nAmount;
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
						nRet = 4;
						break;
					}

					pPlayer->nClubID = nClubID;

				} while (0);

				jsRet["ret"] = nRet;
				sendMsgToPlayer(jsRet, MSG_ROOM_THIRTEEN_APPLAY_DRAG_IN, nSessionID);
			});
		}
		break;

		default:
		{
			return false;
		}
		}

		m_tAutoDismissTimer.clearTime();
		return true;
	}
}

void ThirteenGPrivateRoom::sendRoomMsg(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nOmitSessionID) {
	for (auto& ref : m_vPRooms) {
		if (ref) {
			ref->sendRoomMsg(prealMsg, nMsgType, nOmitSessionID);
		}
	}
}

void ThirteenGPrivateRoom::sendMsgToPlayer(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nSessionID) {
	m_pRoomMgr->sendMsg(prealMsg, nMsgType, getRoomID(), nSessionID, ID_MSG_PORT_CLIENT);
}

bool ThirteenGPrivateRoom::onPlayerNetStateRefreshed(uint32_t nPlayerID, eNetState nState) {
	auto stg = (stgStayPlayer*)isEnterByUserID(nPlayerID);
	if (stg) {
		if (nState != eNet_Offline && isRoomFull() && stg->nState == eNet_Offline && stg->bLeaved) {
			return false;
		}
		stg->nState = nState;
		if (stg->nCurInIdx != (uint16_t)-1 && stg->nCurInIdx < m_vPRooms.size()) {
			m_pRoom = m_vPRooms[stg->nCurInIdx];
			return ThirteenPrivateRoom::onPlayerNetStateRefreshed(nPlayerID, nState);
		}
		return true;
	}
	return false;
}

bool ThirteenGPrivateRoom::onPlayerSetNewSessionID(uint32_t nPlayerID, uint32_t nSessinID) {
	auto stg = (stgStayPlayer*)isEnterByUserID(nPlayerID);
	if (stg) {
		stg->nSessionID = nSessinID;
		if (stg->nCurInIdx != (uint16_t)-1 && stg->nCurInIdx < m_vPRooms.size()) {
			m_pRoom = m_vPRooms[stg->nCurInIdx];
			return ThirteenPrivateRoom::onPlayerSetNewSessionID(nPlayerID, nSessinID);
		}
		return true;
	}
	return false;
}

void ThirteenGPrivateRoom::doSendRoomGameOverInfoToClient(bool isDismissed) {
	ThirteenPrivateRoom::doSendRoomGameOverInfoToClient(isDismissed);
}

bool ThirteenGPrivateRoom::onPlayerDeclineDragIn(uint32_t nUserID) {
	if (m_mStayPlayers.count(nUserID) == 0) {
		return false;
	}
	auto stg = (stgStayPlayer*)m_mStayPlayers[nUserID];
	if (setCoreRoomByUserID(nUserID)) {
		return ThirteenPrivateRoom::onPlayerDeclineDragIn(nUserID);
	}
	else {
		if (stg->isDragIn == false) {
			stg->nClubID = 0;
		}
		if (stg->nState == eNet_Online) {
			Json::Value jsMsg;
			sendMsgToPlayer(jsMsg, MSG_ROOM_THIRTEEN_DECLINE_DRAG_IN, stg->nSessionID);
			jsMsg["idx"] = 0;
			jsMsg["uid"] = stg->nSessionID;
			sendMsgToPlayer(jsMsg, MSG_ROOM_STAND_UP, stg->nSessionID);
		}
		return true;
	}
}

void ThirteenGPrivateRoom::onPlayerAutoStandUp(uint32_t nUserUID, bool bSwitch) {
	auto sPlayer = (stgStayPlayer*)isEnterByUserID(nUserUID);
	if (sPlayer) {
		sPlayer->bAutoStandup = bSwitch;
	}
}

void ThirteenGPrivateRoom::onPlayerAutoLeave(uint32_t nUserUID, bool bSwitch) {
	auto sPlayer = (stgStayPlayer*)isEnterByUserID(nUserUID);
	if (sPlayer) {
		sPlayer->bAutoLeave = bSwitch;
	}
}

void ThirteenGPrivateRoom::doRoomGameOver(bool isDismissed) {
	getRoomRecorder()->setPlayerCnt(getRoomPlayerCnt());
	getRoomRecorder()->setRotBankerPool(m_nRotBankerPool);

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
		}
	}

	if (eState_RoomOvered == m_nPrivateRoomState)  // avoid opotion  loop invoke this function ;
	{
		LOGFMTE("already gave over , why invoker again room id = %u", getRoomID());
		return;
	}
	//doSendRoomGameOverInfoToClient(isDismissed);
	Json::Value jsMsg;
	jsMsg["dismissID"] = m_nApplyDismissUID;
	//jsMsg["result"] = jsArrayPlayers;
	jsMsg["sieralNum"] = getSeiralNum();
	jsMsg["joinAmount"] = getPlayerCnt();
	//sendRoomMsg(jsMsg, MSG_ROOM_GAME_OVER);
	for (auto ref : m_mStayPlayers) {
		auto ref_c = (stgStayPlayer*)ref.second;
		if (ref_c->nState == eNet_Online) {
			sendMsgToPlayer(jsMsg, MSG_ROOM_GAME_OVER, ref_c->nSessionID);
		}
	}

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

bool ThirteenGPrivateRoom::onPlayerDragIn(uint32_t nUserID, uint32_t nClubID, uint32_t nAmount) {
	/*if (nAmount == 0) {
		return false;
	}
	if (m_mStayPlayers.count(nUserID) == 0) {
		return false;
	}
	if (((ThirteenRoom*)getCoreRoom())->onPlayerDragIn(nUserID, nAmount)) {
		m_mStayPlayers[nUserID]->nChip = getCoreRoom()->getPlayerByUID(nUserID)->getChips();
	}*/

	if (m_mStayPlayers.count(nUserID) == 0) {
		return false;
	}
	auto stg = (stgStayPlayer*)m_mStayPlayers[nUserID];
	if (setCoreRoomByUserID(nUserID)) {
		if (ThirteenPrivateRoom::onPlayerDragIn(nUserID, nClubID, nAmount)) {
			stg->bNeedDragIn = false;
			return true;
		}
		return false;
		//return ThirteenPrivateRoom::onPlayerDragIn(nUserID, nClubID, nAmount);
	}
	else {
		if (nAmount == 0) {
			return false;
		}
		if (stg->nClubID && stg->nClubID != nClubID) {
			return false;
		}
		stg->bNeedDragIn = false;
		stg->isDragIn = true;
		stg->nChip += nAmount;
		stg->nAllWrag += nAmount;
		stg->nClubID = nClubID;
		getRoomRecorder()->addDragIn(nUserID, nAmount);
		if (stg->nState == eNet_Online) {
			Json::Value jsMsg;
			jsMsg["chips"] = stg->nChip;
			jsMsg["idx"] = 0;
			sendMsgToPlayer(jsMsg, MSG_ROOM_THIRTEEN_DRAG_IN, stg->nSessionID);
		}
		
		return true;
	}
}

uint16_t ThirteenGPrivateRoom::getPlayerCnt() {
	uint16_t nCnt = 0;
	for (auto ref : m_mStayPlayers) {
		if (((stgStayPlayer*)ref.second)->bLeaved && ((stgStayPlayer*)ref.second)->nState == eNet_Offline) {
			continue;
		}
		nCnt++;
	}
	return nCnt;
}

bool ThirteenGPrivateRoom::dispatcherPlayers(std::vector<stgStayPlayer*>& vWait) {
	auto funDispatcher = [this](std::vector<stgStayPlayer*>& vTemp) {
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
		std::vector<stgStayPlayer*> vTemp;
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
		std::vector<stgStayPlayer*> vTemp;
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

	/*if (vWait.size() > 15) {
		while (vWait.size() >= m_pRoom->getSeatCnt() * 2) {
			std::vector<stgStayPlayer*> vTemp;
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
			std::vector<stgStayPlayer*> vTemp;
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
	}*/
	return true;
}

GameRoom* ThirteenGPrivateRoom::findWaitingRoom() {
	for (auto& ref : m_vPRooms) {
		if (((ThirteenRoom*)ref)->isRoomWaiting()) {
			return ref;
		}
	}
	return nullptr;
}