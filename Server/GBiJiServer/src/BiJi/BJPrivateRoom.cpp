#include "BJPrivateRoom.h"
#include "BiJi\BJRoom.h"
#include "IGamePlayer.h"
#include "IGameRoomState.h"
#include "IGameRoomManager.h"
#include "AsyncRequestQuene.h"
#include "../ServerCommon/ISeverApp.h"
#include "BiJi\BJPlayer.h"
bool BJPrivateRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)
{
	IPrivateRoom::init(pRoomMgr, nSeialNum, nRoomID, nSeatCnt, vJsOpts);
 
	return true;
}

GameRoom* BJPrivateRoom::doCreatRealRoom()
{
	return new BJRoom();
}

uint8_t BJPrivateRoom::getInitRound(uint8_t nLevel)
{
	uint8_t vJun[] = { 10 , 20 , 30 };
	if (nLevel >= sizeof(vJun) / sizeof(uint8_t))
	{
		LOGFMTE("invalid level type = %u", nLevel);
		nLevel = 0;
	}
	return vJun[nLevel];
}

void BJPrivateRoom::doSendRoomGameOverInfoToClient(bool isDismissed)
{
	// send room over msg ;
	Json::Value jsArrayPlayers;
	auto nCnt = getCoreRoom()->getSeatCnt();
	for (uint16_t nIdx = 0; nIdx < nCnt; ++nIdx)
	{
		auto pPlayer = (BJPlayer*)getCoreRoom()->getPlayerByIdx(nIdx);
		if (pPlayer == nullptr)
		{
			continue;
		}
		Json::Value jsPlayer;
		jsPlayer["uid"] = pPlayer->getUserUID();
		jsPlayer["final"] = pPlayer->getChips();
		jsPlayer["lose"] = pPlayer->getPartLose();
		jsPlayer["win"] = pPlayer->getPartWin();
		jsPlayer["xiPai"] = pPlayer->getPartXiPai();
		jsArrayPlayers[jsArrayPlayers.size()] = jsPlayer;
	}

	Json::Value jsMsg;
	jsMsg["dismissID"] = m_nApplyDismissUID;
	jsMsg["result"] = jsArrayPlayers;
	sendRoomMsg(jsMsg, MSG_ROOM_BJ_GAME_OVER);
}

uint8_t BJPrivateRoom::checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)
{
	if (  isRoomStarted() )
	{
		return 7;
	}

	return IPrivateRoom::checkPlayerCanEnter(pEnterRoomPlayer);
}
