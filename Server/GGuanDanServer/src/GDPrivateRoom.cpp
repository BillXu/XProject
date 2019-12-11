#include "GDPrivateRoom.h"
#include "GDRoom.h"
#include "GDPlayer.h"
#include "IGamePlayer.h"
#include "IGameRoomState.h"
#include "IGameRoomManager.h"
#include "AsyncRequestQuene.h"
#include "../ServerCommon/ISeverApp.h"
#include "IGameOpts.h"
bool GDPrivateRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, std::shared_ptr<IGameOpts> ptrGameOpts)
{
	IPrivateRoom::init(pRoomMgr, nSeialNum, nRoomID, ptrGameOpts);
	//m_nAutoOpenCnt = vJsOpts["starGame"].asUInt();
	return true;
}

GameRoom* GDPrivateRoom::doCreatRealRoom()
{
	return new GDRoom();
}

//uint8_t GDPrivateRoom::getInitRound(uint8_t nLevel)
//{
//	uint8_t vJun[] = { 9 , 18 , 27 , 8 , 16 };
//	if (nLevel >= sizeof(vJun) / sizeof(uint8_t))
//	{
//		LOGFMTE("invalid level type = %u", nLevel);
//		nLevel = 0;
//	}
//	return vJun[nLevel];
//}

void GDPrivateRoom::doSendRoomGameOverInfoToClient(bool isDismissed)
{
	// send room over msg ;
	Json::Value jsArrayPlayers;
	auto nCnt = getCoreRoom()->getSeatCnt();
	for (uint16_t nIdx = 0; nIdx < nCnt; ++nIdx)
	{
		auto pPlayer = (GDPlayer*)getCoreRoom()->getPlayerByIdx(nIdx);
		if (pPlayer == nullptr)
		{
			continue;
		}
		Json::Value jsPlayer;
		jsPlayer["uid"] = pPlayer->getUserUID();
		jsPlayer["final"] = pPlayer->getChips();
		jsPlayer["extraTime"] = pPlayer->getExtraTime();
		jsArrayPlayers[jsArrayPlayers.size()] = jsPlayer;
	}

	Json::Value jsMsg;
	jsMsg["dismissID"] = m_nApplyDismissUID;
	jsMsg["result"] = jsArrayPlayers;
	sendRoomMsg(jsMsg, MSG_ROOM_GAME_OVER);
}

bool GDPrivateRoom::canStartGame(IGameRoom* pRoom)
{
	if (m_isOpen == false && getOpts()->getAutoStartCnt() > 0)
	{
		uint8_t nCnt = 0;
		auto pNRoom = ((GDRoom*)pRoom);
		for (uint8_t nIdx = 0; nIdx < pNRoom->getSeatCnt(); ++nIdx)
		{
			if (pNRoom->getPlayerByIdx(nIdx))
			{
				++nCnt;
			}
		}

		m_isOpen = nCnt >= getOpts()->getAutoStartCnt();
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

uint8_t GDPrivateRoom::checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer) {
	if (m_pRoom)
	{
		return m_pRoom->checkPlayerCanEnter(pEnterRoomPlayer);
	}
	LOGFMTE("private room can not room is null rooom id = %u", getRoomID());
	return 1;
}