#include "MJPrivateRoom.h"
#include "log4z.h"
#include "FXMJRoom.h"
#include "FXMJPlayer.h"
bool MJPrivateRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts) {
	m_mPreSitIdxes.clear();
	return IPrivateRoom::init(pRoomMgr, nSeialNum, nRoomID, nSeatCnt, vJsOpts);
}

GameRoom* MJPrivateRoom::doCreatRealRoom()
{
	return new FXMJRoom();
}

uint8_t MJPrivateRoom::getInitRound(uint8_t nLevel)
{
	if (nLevel > 3) {
		return 6;
	}
	uint8_t vRounds[8] = { 1, 2, 3, 4, 6, 12, 18, 24 };
	return vRounds[nLevel];
}

void MJPrivateRoom::doSendRoomGameOverInfoToClient(bool isDismissed)
{
	Json::Value jsMsg, jsPlayers;
	auto pRoom = getCoreRoom();
	for (uint8_t nIdx = 0; nIdx < pRoom->getSeatCnt(); ++nIdx)
	{
		auto pp = (FXMJPlayer*)pRoom->getPlayerByIdx(nIdx);
		if (pp == nullptr)
		{
			LOGFMTE("why private player is null room id = %u idx = %u", getRoomID(), nIdx);
			continue;
		}

		Json::Value jsPlayer;
		jsPlayer["uid"] = pp->getUserUID();
		jsPlayer["final"] = pp->getChips();
		jsPlayer["anGangCnt"] = pp->getAnGangCnt();
		jsPlayer["cycloneCnt"] = pp->getHuaGangCnt();
		jsPlayer["dianPaoCnt"] = pp->getDianPaoCnt();
		jsPlayer["huCnt"] = pp->getHuCnt();
		jsPlayer["mingGangCnt"] = pp->getMingGangCnt();
		jsPlayer["ZMCnt"] = pp->getZiMoCnt();
		jsPlayer["bankerCnt"] = pp->getBankerCnt();
		jsPlayer["coolCnt"] = pp->getCoolCnt();
		jsPlayers[jsPlayers.size()] = jsPlayer;
	}
	jsMsg["dismissID"] = m_nApplyDismissUID;
	jsMsg["result"] = jsPlayers;
	sendRoomMsg(jsMsg, MSG_ROOM_GAME_OVER);
}

bool MJPrivateRoom::canStartGame(IGameRoom* pRoom) {
	if (isClubRoom() && m_isOpen == false) {
		if (getPlayerCnt() == getSeatCnt()) {
			m_isOpen = true;
		}
	}
	return m_isOpen;
}

bool MJPrivateRoom::isCircle() {
	auto pRoom = (FXMJRoom*)getCoreRoom();
	return pRoom->isCircle();
}

void MJPrivateRoom::decreaseLeftRound() {
	if (isCircle()) {
		auto pRoom = (FXMJRoom*)getCoreRoom();
		if (pRoom->isOneCircleEnd()) {
			pRoom->clearOneCircleEnd();
		}
		else {
			return;
		}
	}
	
	IPrivateRoom::decreaseLeftRound();
}

bool MJPrivateRoom::applyDoDismissCheck() {
	return m_vPlayerAgreeDismissRoom.size() + 1 >= getPlayerCnt();
}

bool MJPrivateRoom::onPlayerEnter(stEnterRoomData* pEnterRoomPlayer) {
	//TODO
	if (m_mPreSitIdxes.count(pEnterRoomPlayer->nUserUID)) {
		pEnterRoomPlayer->nPreSitIdx = m_mPreSitIdxes[pEnterRoomPlayer->nUserUID];
	}

	return IPrivateRoom::onPlayerEnter(pEnterRoomPlayer);
}

void MJPrivateRoom::onPlayerSitDown(IGameRoom* pRoom, IGamePlayer* pPlayer) {
	//TODO
	m_mPreSitIdxes[pPlayer->getUserUID()] = pPlayer->getIdx();
}