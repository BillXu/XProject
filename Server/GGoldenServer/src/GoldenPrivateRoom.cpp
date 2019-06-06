#include "GoldenPrivateRoom.h"
#include "Golden\GoldenRoom.h"
#include "IGamePlayer.h"
#include "IGameRoomState.h"
#include "IGameRoomManager.h"
#include "AsyncRequestQuene.h"
#include "../ServerCommon/ISeverApp.h"
#include "IPokerOpts.h"
bool GoldenPrivateRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, std::shared_ptr<IGameOpts> ptrGameOpts)
{
	IPrivateRoom::init(pRoomMgr, nSeialNum, nRoomID, ptrGameOpts);
	/*m_isForbitEnterRoomWhenStarted = vJsOpts["forbidJoin"].asUInt() == 1;
	m_nAutoOpenCnt = vJsOpts["starGame"].asUInt();*/
	return true;
}

uint8_t GoldenPrivateRoom::checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)
{
	if (std::dynamic_pointer_cast<IPokerOpts>(getOpts())->isForbitEnterRoomWhenStarted() && isRoomStarted() )
	{
		return 7;
	}

	return IPrivateRoom::checkPlayerCanEnter(pEnterRoomPlayer);
}

GameRoom* GoldenPrivateRoom::doCreatRealRoom()
{
	return new GoldenRoom();
}

//uint8_t GoldenPrivateRoom::getInitRound( uint8_t nLevel )
//{
//	uint8_t vJun[] = { 10 , 20 , 30 };
//	if ( nLevel >= sizeof(vJun) / sizeof(uint8_t) )
//	{
//		LOGFMTE( "invalid level type = %u",nLevel );
//		nLevel = 0;
//	}
//	return vJun[nLevel];
//}

void GoldenPrivateRoom::doSendRoomGameOverInfoToClient( bool isDismissed )
{
	// send room over msg ;
	Json::Value jsArrayPlayers;
	auto nCnt = getCoreRoom()->getSeatCnt();
	for ( uint16_t nIdx = 0; nIdx < nCnt; ++nIdx)
	{
		auto pPlayer = getCoreRoom()->getPlayerByIdx(nIdx);
		if (pPlayer == nullptr)
		{
			continue;
		}
		Json::Value jsPlayer;
		jsPlayer["uid"] = pPlayer->getUserUID();
		jsPlayer["final"] = pPlayer->getChips();
		jsArrayPlayers[jsArrayPlayers.size()] = jsPlayer;
	}

	Json::Value jsMsg;
	jsMsg["dismissID"] = m_nApplyDismissUID;
	jsMsg["result"] = jsArrayPlayers;
	sendRoomMsg(jsMsg, MSG_ROOM_GAME_OVER );
}

bool GoldenPrivateRoom::canStartGame(IGameRoom* pRoom)
{
	if ( m_isOpen == false && getOpts()->getAutoStartCnt() > 0)
	{
		uint8_t nCnt = 0;
		auto pGoldenRoom = ((GoldenRoom*)pRoom);
		for (uint8_t nIdx = 0; nIdx < getOpts()->getSeatCnt(); ++nIdx)
		{
			if (pGoldenRoom->getPlayerByIdx(nIdx))
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