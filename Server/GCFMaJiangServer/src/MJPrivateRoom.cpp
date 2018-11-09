#include "MJPrivateRoom.h"
#include "log4z.h"
#include "CFMJRoom.h"
#include "CFMJPlayer.h"
GameRoom* MJPrivateRoom::doCreatRealRoom()
{
	return new CFMJRoom();
}

uint8_t MJPrivateRoom::getInitRound(uint8_t nLevel)
{
#ifdef _DEBUG
	return 2;
#endif // _DEBUG

	if (isCircle()) {
		if (nLevel < 2 || nLevel > 5) {
			nLevel = 2;
		}
	}
	else {
		if (nLevel > 1) {
			nLevel = 0;
		}
	}

	uint8_t vRounds[6] = { 8, 16, 1, 2, 3, 4 };
	return vRounds[nLevel];
}

void MJPrivateRoom::doSendRoomGameOverInfoToClient(bool isDismissed)
{
	Json::Value jsMsg, jsPlayers;
	auto pRoom = getCoreRoom();
	for (uint8_t nIdx = 0; nIdx < pRoom->getSeatCnt(); ++nIdx)
	{
		auto pp = (CFMJPlayer*)pRoom->getPlayerByIdx(nIdx);
		if (pp == nullptr)
		{
			LOGFMTE("why private player is null room id = %u idx = %u", getRoomID(), nIdx);
			continue;
		}

		Json::Value jsPlayer;
		jsPlayer["uid"] = pp->getUserUID();
		jsPlayer["final"] = pp->getChips();
		jsPlayer["anGangCnt"] = pp->getAnGangCnt();
		jsPlayer["dianPaoCnt"] = pp->getDianPaoCnt();
		jsPlayer["huCnt"] = pp->getHuCnt();
		jsPlayer["mingGangCnt"] = pp->getMingGangCnt();
		jsPlayer["ZMCnt"] = pp->getZiMoCnt();
		jsPlayer["extraTime"] = pp->getExtraTime();
		jsPlayers[jsPlayers.size()] = jsPlayer;
	}
	jsMsg["dismissID"] = m_nApplyDismissUID;
	jsMsg["result"] = jsPlayers;
	sendRoomMsg(jsMsg, MSG_ROOM_GAME_OVER);
}

bool MJPrivateRoom::canStartGame(IGameRoom* pRoom) {
	return true;
}

bool MJPrivateRoom::isCircle() {
	auto pRoom = (CFMJRoom*)getCoreRoom();
	return pRoom->isCircle();
}

void MJPrivateRoom::decreaseLeftRound() {
	if (isCircle()) {
		auto pRoom = (CFMJRoom*)getCoreRoom();
		if (pRoom->isOneCircleEnd()) {
			pRoom->clearOneCircleEnd();
		}
		else {
			return;
		}
	}

	IPrivateRoom::decreaseLeftRound();
}