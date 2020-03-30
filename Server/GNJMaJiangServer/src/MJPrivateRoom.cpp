#include "MJPrivateRoom.h"
#include "log4z.h"
#include "NJMJRoom.h"
#include "NJMJPlayer.h"
GameRoom* MJPrivateRoom::doCreatRealRoom()
{
	return new NJMJRoom();
}

void MJPrivateRoom::doSendRoomGameOverInfoToClient(bool isDismissed)
{
	Json::Value jsMsg, jsPlayers;
	auto pRoom = (NJMJRoom*)getCoreRoom();
	for (uint8_t nIdx = 0; nIdx < pRoom->getSeatCnt(); ++nIdx)
	{
		auto pp = (NJMJPlayer*)pRoom->getPlayerByIdx(nIdx);
		if (pp == nullptr)
		{
			LOGFMTE("why private player is null room id = %u idx = %u", getRoomID(), nIdx);
			continue;
		}

		Json::Value jsPlayer;
		jsPlayer["uid"] = pp->getUserUID();
		jsPlayer["final"] = pp->getChips();
		jsPlayer["realFinal"] = 0; //TODO
		jsPlayer["anGangCnt"] = pp->getAnGangCnt();
		jsPlayer["dianPaoCnt"] = pp->getDianPaoCnt();
		jsPlayer["huCnt"] = pp->getHuCnt();
		jsPlayer["mingGangCnt"] = pp->getMingGangCnt();
		jsPlayer["ZMCnt"] = pp->getZiMoCnt();
		jsPlayer["bestCards"] = pp->getBestCards();
		jsPlayer["extraTime"] = pp->getExtraTime();
		jsPlayer["baoMi"] = pp->isBaoMi() ? 1 : 0;
		if (pRoom->isEnableWaiBao()) {
			jsPlayer["extraOffset"] = pp->getExtraOffset();
		}
		jsPlayers[jsPlayers.size()] = jsPlayer;
	}
	jsMsg["dismissID"] = m_nApplyDismissUID;
	jsMsg["result"] = jsPlayers;
	sendRoomMsg(jsMsg, MSG_ROOM_GAME_OVER);
}

bool MJPrivateRoom::canStartGame(IGameRoom* pRoom) {
	return true;
}