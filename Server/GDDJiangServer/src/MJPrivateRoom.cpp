#include "MJPrivateRoom.h"
#include "log4z.h"
#include "DDMJRoom.h"
#include "DDMJPlayer.h"
#include "DDMJOpts.h"
GameRoom* MJPrivateRoom::doCreatRealRoom()
{
	return new DDMJRoom();
}

void MJPrivateRoom::doSendRoomGameOverInfoToClient(bool isDismissed)
{
	Json::Value jsMsg, jsPlayers;
	auto pRoom = getCoreRoom();
	for (uint8_t nIdx = 0; nIdx < pRoom->getSeatCnt(); ++nIdx)
	{
		auto pp = (DDMJPlayer*)pRoom->getPlayerByIdx(nIdx);
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
		jsPlayer["bestCards"] = pp->getBestCards();
		jsPlayer["bestChips"] = pp->getBestChips();
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

void MJPrivateRoom::onStartGame(IGameRoom* pRoom) {
	IPrivateRoom::onStartGame(pRoom);
	if (m_nLeftRounds == getOpts()->getInitRound())
	{
		auto pGameOpts = std::dynamic_pointer_cast<DDMJOpts>(getOpts());
		if (pGameOpts->isRandomChangeSeat()) {
			((DDMJRoom*)pRoom)->doRandomChangeSeat();
		}
	}
}

uint8_t MJPrivateRoom::checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer) {
	if (m_pRoom)
	{
		return m_pRoom->checkPlayerCanEnter(pEnterRoomPlayer);
	}
	LOGFMTE("private room can not room is null rooom id = %u", getRoomID());
	return 1;
}