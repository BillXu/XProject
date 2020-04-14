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
	m_nAutoOpenCnt = vJsOpts["starGame"].asUInt();
	m_stopEnterWhenOpen = vJsOpts["stopEnterWhenOpen"].isNull() == false && vJsOpts["stopEnterWhenOpen"].asInt() == 1;
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

	Json::StyledWriter writerJs;
	std::string strContent = writerJs.write(jsMsg);
	LOGFMTD( "gameover room id = %d  ret: %s " , getCoreRoom()->getRoomID(),strContent.c_str() );
}

uint8_t BJPrivateRoom::checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)
{
	if ( m_stopEnterWhenOpen && isRoomStarted() )
	{
		return 7;
	}

	return IPrivateRoom::checkPlayerCanEnter(pEnterRoomPlayer);
}

bool BJPrivateRoom::canStartGame(IGameRoom* pRoom)
{
	if ( m_nAutoOpenCnt > 0 )
	{
		uint8_t nCnt = 0;
		auto pNRoom = ((BJRoom*)pRoom);
		for (uint8_t nIdx = 0; nIdx < pNRoom->getSeatCnt(); ++nIdx)
		{
			if (pNRoom->getPlayerByIdx(nIdx))
			{
				++nCnt;
			}
		}

		auto before = m_isOpen;
		m_isOpen = nCnt >= m_nAutoOpenCnt ;
		if (m_isOpen && before == false )
		{
			m_isOpen = true;
			Json::Value js;
			sendRoomMsg(js, MSG_ROOM_DO_OPEN);
			LOGFMTI(" room id = %u auto set open", getRoomID());
		}
	}

	return IPrivateRoom::canStartGame(pRoom);
}
