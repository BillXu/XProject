#include "MJPrivateRoom.h"
#include "log4z.h"
#include "FXMJRoom.h"
#include "FXMJPlayer.h"
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