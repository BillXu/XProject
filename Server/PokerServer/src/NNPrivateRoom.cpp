#include "NNPrivateRoom.h"
#include "NiuNiu\NNRoom.h"
#include "IGamePlayer.h"
#include "IGameRoomState.h"
#include "IGameRoomManager.h"
#include "AsyncRequestQuene.h"
#include "../ServerCommon/ISeverApp.h"
bool NNPrivateRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)
{
	IPrivateRoom::init(pRoomMgr, nSeialNum, nRoomID, nSeatCnt, vJsOpts);
	m_isForbitEnterRoomWhenStarted = vJsOpts["forbidJoin"].asUInt() == 1;
	m_isEnableWhiteList = vJsOpts["enableWhiteList"].asUInt() == 1;
	m_nAutoOpenCnt = vJsOpts["starGame"].asUInt();
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

bool NNPrivateRoom::onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)
{
	// check white list ;
	if ( nMsgType == MSG_PLAYER_SIT_DOWN && m_isEnableWhiteList )
	{
		auto p = getCoreRoom()->getStandPlayerBySessionID(nSessionID);
		if ( nullptr == p || p->nUserUID == m_nOwnerUID || getCoreRoom()->isRoomFull() ) // room owner skip white list check ;
		{
			return IPrivateRoom::onMsg(prealMsg, nMsgType, eSenderPort, nSessionID);
		}

		// do check white list 
		Json::Value js;
		js["listOwner"] = m_nOwnerUID;
		js["checkUID"] = p->nUserUID;
		auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
		auto pRoomMgr = m_pRoomMgr;
		auto nRoomID = getRoomID();
		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, m_nOwnerUID, eAsync_Check_WhiteList, js, [nSessionID, pRoomMgr, nRoomID]( uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut )
		{
			if (isTimeOut)
			{
				LOGFMTE( "room id = %u session id = %u check white list timeout",nRoomID,nSessionID );
				jsUserData["ret"] = 7;
				pRoomMgr->sendMsg(jsUserData, MSG_PLAYER_SIT_DOWN, nRoomID, nSessionID, ID_MSG_PORT_CLIENT);
				return;
			}

			auto nRet = retContent["ret"].asUInt();
			if ( nRet )
			{
				jsUserData["ret"] = 7;
				pRoomMgr->sendMsg(jsUserData, MSG_PLAYER_SIT_DOWN, nRoomID, nSessionID, ID_MSG_PORT_CLIENT);
				return;
			}

			auto pRoom = (IPrivateRoom*)pRoomMgr->getRoomByID(nRoomID);
			if (pRoom == nullptr)
			{
				LOGFMTE( "after check white list , room is null id = %u",nRoomID );
				jsUserData["ret"] = 6;
				pRoomMgr->sendMsg(jsUserData, MSG_PLAYER_SIT_DOWN, nRoomID, nSessionID, ID_MSG_PORT_CLIENT);
				return;
			}
			// do go normal sitdown ;  only jin hua niu niu process here , mj can not do like this ;
			pRoom->getCoreRoom()->onMsg(jsUserData, MSG_PLAYER_SIT_DOWN, ID_MSG_PORT_CLIENT, nSessionID);
		},prealMsg,p->nUserUID);
		return true;
	}
	return IPrivateRoom::onMsg(prealMsg,nMsgType,eSenderPort,nSessionID );
}

bool NNPrivateRoom::canStartGame(IGameRoom* pRoom)
{
	if ( m_isOpen == false && m_nAutoOpenCnt > 0)
	{
		uint8_t nCnt = 0;
		auto pNRoom = ((NNRoom*)pRoom);
		for (uint8_t nIdx = 0; nIdx < pNRoom->getSeatCnt(); ++nIdx)
		{
			if (pNRoom->getPlayerByIdx(nIdx))
			{
				++nCnt;
			}
		}

		m_isOpen = nCnt >= m_nAutoOpenCnt;
	}

	return IPrivateRoom::canStartGame(pRoom);
}