#include "NNPrivateRoom.h"
#include "NiuNiu\NNRoom.h"
#include "IGamePlayer.h"
#include "IGameRoomState.h"
bool NNPrivateRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)
{
	IPrivateRoom::init(pRoomMgr, nSeialNum, nRoomID, nSeatCnt, vJsOpts);
	m_isForbitEnterRoomWhenStarted = vJsOpts["forbidJoin"].asUInt() == 1;
	return true;
}

uint8_t NNPrivateRoom::checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)
{
	if (m_isForbitEnterRoomWhenStarted && isRoomStarted() )
	{
		return 7;
	}

	return IPrivateRoom::checkPlayerCanEnter(pEnterRoomPlayer);
}

GameRoom* NNPrivateRoom::doCreatRealRoom()
{
	return new NNRoom();
}

uint8_t NNPrivateRoom::getInitRound( uint8_t nLevel )
{
	uint8_t vJun[] = { 10 , 20 , 30 };
	if ( nLevel >= sizeof(vJun) / sizeof(uint8_t) )
	{
		LOGFMTE( "invalid level type = %u",nLevel );
		nLevel = 0;
	}
	return vJun[nLevel];
}

void NNPrivateRoom::doSendRoomGameOverInfoToClient( bool isDismissed )
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