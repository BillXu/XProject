#include "IPrivateRoom.h"
#include "log4z.h"
#include "IGameRoomManager.h"
#include "IGameRoomState.h"
#include "IGamePlayer.h"
#include "ISeverApp.h"
#include "AsyncRequestQuene.h"
#include <time.h>
#define TIME_WAIT_REPLY_DISMISS 180
#define TIME_AUTO_DISMISS (60*60*1)
IPrivateRoom::~IPrivateRoom()
{
	delete m_pRoom;
	m_pRoom = nullptr;
}

bool IPrivateRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts )
{
	m_pRoom = doCreatRealRoom();
	if (!m_pRoom)
	{
		LOGFMTE("create private room error ");
		return false;
	}
	LOGFMTD("create 1 private room");
	auto bRet = m_pRoom->init(pRoomMgr, nSeialNum, nRoomID, nSeatCnt, vJsOpts);
	if (!bRet)
	{
		LOGFMTE("init private room error ");
		return false;
	}
	m_pRoom->setDelegate(this);

	// init member 
	m_pRoomMgr = pRoomMgr;
	m_isForFree = vJsOpts["isFree"].asUInt() == 1;
	m_isAA = vJsOpts["isAA"].asUInt() == 1;
	m_nOwnerUID = vJsOpts["uid"].asUInt();
	m_nRoundLevel = vJsOpts["level"].asUInt();
	m_isEnableWhiteList = vJsOpts["enableWhiteList"].isNull() == false && vJsOpts["enableWhiteList"].asUInt() == 1;
	m_nLeftRounds = getInitRound(m_nRoundLevel);

	m_isOneRoundNormalEnd = false;
	m_nPrivateRoomState = eState_WaitStart;
	m_bWaitDismissReply = false;

	m_vPlayerAgreeDismissRoom.clear();
	m_tInvokerTime = 0;
	m_nApplyDismissUID = 0;
	m_isOpen = false;

	// start auto dismiss timer ;
	m_tAutoDismissTimer.reset();
	m_tAutoDismissTimer.setInterval(TIME_AUTO_DISMISS);
#ifdef _DEBUG
	m_tAutoDismissTimer.setInterval( 60*5 );
#endif // _DEBUG

	m_tAutoDismissTimer.setIsAutoRepeat(false);
	m_tAutoDismissTimer.setCallBack([this](CTimer*p, float f) {
		m_tInvokerTime = 0;
		m_nApplyDismissUID = 0;
		LOGFMTI("system auto dismiss room id = %u , owner id = %u",getRoomID(), m_nOwnerUID );
		doRoomGameOver(true);
	});
	m_tAutoDismissTimer.start();
	return true;
}

bool IPrivateRoom::onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)
{
	pEnterRoomPlayer->nChip = 0;
	if (m_pRoom == nullptr)
	{
		LOGFMTE("why room is null ? ");
		return false;
	}
	if (m_pRoom->onPlayerEnter(pEnterRoomPlayer))
	{
		
	}
	return true;
}

uint8_t IPrivateRoom::checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)
{
	//if ( isRoomStarted() )
	//{
	//	return 7;
	//}

	if ( m_isAA && m_isForFree == false && pEnterRoomPlayer->nDiamond < getDiamondNeed(m_nRoundLevel, m_isAA))
	{
		// diamond is not enough 
		return 3;
	}

	if ( m_pRoom )
	{
		return m_pRoom->checkPlayerCanEnter(pEnterRoomPlayer);
	}
	LOGFMTE("private room can not room is null rooom id = %u",getRoomID());
	return 1;
}

bool IPrivateRoom::isRoomFull()
{
	return m_pRoom->isRoomFull();
}

bool IPrivateRoom::doDeleteRoom()
{
	m_tWaitReplyDismissTimer.canncel();
	m_tAutoDismissTimer.canncel();
	// tell client closed room ;
	Json::Value jsDoClosed;
	jsDoClosed["roomID"] = getRoomID();
	jsDoClosed["isDismiss"] = m_nApplyDismissUID > 0 ? 1 : 0;
	sendRoomMsg(jsDoClosed, MSG_VIP_ROOM_DO_CLOSED);

	// tell data svr , the room is closed 
	Json::Value jsReqInfo;
	jsReqInfo["targetUID"] = m_nOwnerUID;
	jsReqInfo["roomID"] = getRoomID();
	jsReqInfo["port"] = m_pRoomMgr->getSvrApp()->getLocalSvrMsgPortType();
	auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
	pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, m_nOwnerUID, eAsync_Inform_RoomDeleted, jsReqInfo);
	return m_pRoom->doDeleteRoom();
}

//virtual void roomItemDetailVisitor(Json::Value& vOutJsValue) = 0;
uint32_t IPrivateRoom::getRoomID()
{
	return m_pRoom->getRoomID();
}
 
uint32_t IPrivateRoom::getSeiralNum()
{
	return m_pRoom->getSeiralNum();
}

void IPrivateRoom::update(float fDelta)
{
	if ( m_pRoom )
	{
		m_pRoom->update(fDelta);
	}
}

bool IPrivateRoom::onProcessWhiteListSitDown(Json::Value& prealMsg, uint32_t nSessionID)
{
	auto p = getCoreRoom()->getStandPlayerBySessionID(nSessionID);
	if ( nullptr == p || p->nUserUID == m_nOwnerUID || getCoreRoom()->isRoomFull()) // room owner skip white list check ;
	{
		return false;
	}

	// do check white list 
	Json::Value js;
	js["listOwner"] = m_nOwnerUID;
	js["checkUID"] = p->nUserUID;
	auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
	auto pRoomMgr = m_pRoomMgr;
	auto nRoomID = getRoomID();
	pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, m_nOwnerUID, eAsync_Check_WhiteList, js, [nSessionID, pRoomMgr, nRoomID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
	{
		if (isTimeOut)
		{
			LOGFMTE("room id = %u session id = %u check white list timeout", nRoomID, nSessionID);
			jsUserData["ret"] = 7;
			pRoomMgr->sendMsg(jsUserData, MSG_PLAYER_SIT_DOWN, nRoomID, nSessionID, ID_MSG_PORT_CLIENT);
			return;
		}

		auto nRet = retContent["ret"].asUInt();
		if (nRet)
		{
			jsUserData["ret"] = 7;
			pRoomMgr->sendMsg(jsUserData, MSG_PLAYER_SIT_DOWN, nRoomID, nSessionID, ID_MSG_PORT_CLIENT);
			return;
		}

		auto pRoom = (IPrivateRoom*)pRoomMgr->getRoomByID(nRoomID);
		if (pRoom == nullptr)
		{
			LOGFMTE("after check white list , room is null id = %u", nRoomID);
			jsUserData["ret"] = 6;
			pRoomMgr->sendMsg(jsUserData, MSG_PLAYER_SIT_DOWN, nRoomID, nSessionID, ID_MSG_PORT_CLIENT);
			return;
		}
		// do go normal sitdown ;  only jin hua niu niu process here , mj can not do like this ;
		pRoom->getCoreRoom()->onMsg(jsUserData, MSG_PLAYER_SIT_DOWN, ID_MSG_PORT_CLIENT, nSessionID);
	}, prealMsg, p->nUserUID);
	return true;
}

bool IPrivateRoom::onMsg( Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID )
{
	if (nMsgType == MSG_PLAYER_SIT_DOWN && m_isEnableWhiteList )
	{
		if ( onProcessWhiteListSitDown(prealMsg, nSessionID) )
		{
			return true;
		}
	}

	switch (nMsgType)
	{
	case MSG_REQUEST_ROOM_INFO:
	{
		LOGFMTD("reback room state and info msg to session id =%u", nSessionID);
		sendRoomInfo(nSessionID);
	}
	break;
	case MSG_PLAYER_OPEN_ROOM:
	{
		m_isOpen = true;
		Json::Value js;
		sendRoomMsg(js, MSG_ROOM_DO_OPEN);
		LOGFMTI(" room id = %u do set open",getRoomID() );
	}
	break;
	case MSG_PLAYER_STAND_UP:
	case MSG_PLAYER_LEAVE_ROOM:
	{
		auto pp = m_pRoom->getPlayerBySessionID(nSessionID);
		if ( isRoomStarted() && pp ) // if game start , and you are sit down , you can not direct leave , if you not sit down , you can leave 
		{
			LOGFMTE("why you leave room ? already start can not leave , just apply dissmiss room id = %u , sessionID = %u", getRoomID(), nSessionID);
			return true;
		}
		else
		{
			return m_pRoom->onMsg(prealMsg, nMsgType, eSenderPort, nSessionID);
		}
	}
	break;
	case MSG_PLAYER_CHAT_MSG:
	{
		auto pp = m_pRoom->getPlayerBySessionID(nSessionID);
		if (pp == nullptr)
		{
			prealMsg["ret"] = 1;
			sendMsgToPlayer(prealMsg, MSG_PLAYER_CHAT_MSG, nSessionID);
			break;
		}

		prealMsg["playerIdx"] = pp->getIdx();
		sendRoomMsg(prealMsg, MSG_ROOM_CHAT_MSG);
	}
	break;
	case MSG_APPLY_DISMISS_VIP_ROOM:
	{
		auto pp = m_pRoom->getPlayerBySessionID(nSessionID);
		auto nApplyUID = prealMsg["uid"].asUInt();
		if ( pp )
		{
			nApplyUID = pp->getUserUID();
		}

		if ( isRoomStarted() == false )
		{
			if ( nApplyUID != m_nOwnerUID )
			{
				LOGFMTE( "client shoud not send this msg , room id = %u not start , you are not room owner, so you can not dismiss player id = %u, you can leave",getRoomID(), nApplyUID );
				return true;
			}
			
			doRoomGameOver(true);
			return true;
		}

		if ( nullptr == pp && nApplyUID != m_nOwnerUID )
		{
			LOGFMTE("you are not in room , and you are not owner  uid = %u",nApplyUID );
			return true;
		}

		if ( pp )
		{
			m_vPlayerAgreeDismissRoom[nApplyUID] = 1;
		}
		
		if (m_bWaitDismissReply)
		{
			LOGFMTE("client should not send this msg ,already waiting reply %u why you go on apply ?", nApplyUID);
			return false;
		}
		m_bWaitDismissReply = true;
		m_tInvokerTime = time(nullptr);
		m_nApplyDismissUID = nApplyUID;

		Json::Value jsMsg;
		jsMsg["applyerIdx"] = pp == nullptr ? -1 : pp->getIdx();
		sendRoomMsg(jsMsg, MSG_ROOM_APPLY_DISMISS_VIP_ROOM );

		// start wait timer ;
		m_tWaitReplyDismissTimer.reset();
		m_tWaitReplyDismissTimer.setInterval(TIME_WAIT_REPLY_DISMISS);
		m_tWaitReplyDismissTimer.setIsAutoRepeat(false);
		m_tWaitReplyDismissTimer.setCallBack([this](CTimer*p, float f) {
			doRoomGameOver(true);
		});
		m_tWaitReplyDismissTimer.start();
	}
	break;
	case MSG_REPLY_DISSMISS_VIP_ROOM_APPLY:
	{
		if (!m_bWaitDismissReply)
		{
			LOGFMTE("nobody apply to dismiss room ,why you reply ? session id = %u", nSessionID);
			break;
		}

		auto pp = m_pRoom->getPlayerBySessionID(nSessionID);
		if (pp == nullptr)
		{
			LOGFMTE("pp is null why you replay dismiss ,you are not sit in room, session id = %u", nSessionID);
			break;
		}

		LOGFMTD("received player session id = %u , reply dismiss ret = %u", nSessionID, prealMsg["reply"].asUInt());
		m_vPlayerAgreeDismissRoom[pp->getUserUID()] = prealMsg["reply"].asUInt();

		// tell client result ;
		Json::Value jsMsg;
		jsMsg["idx"] = pp->getIdx();
		jsMsg["reply"] = prealMsg["reply"];
		sendRoomMsg(jsMsg, MSG_ROOM_REPLY_DISSMISS_VIP_ROOM_APPLY);

		auto isAgree = prealMsg["reply"].asUInt() == 1;
		if ( isAgree == false ) // some one do not agree dissmiss room ; so canncel dismiss room act ;
		{
			m_bWaitDismissReply = 0;
			m_tInvokerTime = 0;
			m_vPlayerAgreeDismissRoom.clear();
			m_tWaitReplyDismissTimer.canncel();
			m_tWaitReplyDismissTimer.reset();
			m_nApplyDismissUID = 0;
			LOGFMTE("player = %u do not agree dismiss room id = %u , so cannecl this act",pp->getUserUID(),getRoomID() );
			break;
		}

		if ( m_vPlayerAgreeDismissRoom.size() == getRoomPlayerCnt() )
		{
			m_tWaitReplyDismissTimer.canncel();
			m_tWaitReplyDismissTimer.reset();
			doRoomGameOver(true);
		}
	}
	break;
	case MSG_REQ_ROOM_ITEM_INFO:
	{
		Json::Value jsMsg;
		getCoreRoom()->packRoomInfo(jsMsg);
		jsMsg["isOpen"] = m_isOpen ? 1 : 0;
		Json::Value jsArrayUIDs;
		for (uint32_t nIdx = 0; nIdx < getCoreRoom()->getSeatCnt(); ++nIdx)
		{
			auto p = getCoreRoom()->getPlayerByIdx(nIdx);
			if (p == nullptr)
			{
				continue;
			}
			jsArrayUIDs[jsArrayUIDs.size()] = p->getUserUID();
		}
		jsMsg["players"] = jsArrayUIDs;
		sendMsgToPlayer(jsMsg, nMsgType, nSessionID );
	}
	break;
	default:
		if (m_pRoom && m_pRoom->onMsg(prealMsg, nMsgType, eSenderPort, nSessionID) )
		{
			m_tAutoDismissTimer.clearTime(); // recieved client msg , auto reset timer ticket count , count from 0 again ;
		}
		else
		{
			if (!m_pRoom)
			{
				LOGFMTE( "why private room core Room is null" );
				return false;
			}

			prealMsg["roomState"] = getCoreRoom()->getCurState()->getStateID();
			LOGFMTE( "room id = %u do not process msg = %u , room state = %u idx = %u",getRoomID(),nMsgType,getCoreRoom()->getCurState()->getStateID(), getCoreRoom()->getCurState()->getCurIdx() );
			return false;
		}
	}

	m_tAutoDismissTimer.clearTime(); // recieved client msg , auto reset timer ticket count , count from 0 again ;
	return true;
}

void IPrivateRoom::sendRoomMsg(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nOmitSessionID )
{
	if (m_pRoom)
	{
		m_pRoom->sendRoomMsg(prealMsg,nMsgType, nOmitSessionID );
	}
}

void IPrivateRoom::sendMsgToPlayer(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nSessionID)
{
	if (m_pRoom)
	{
		m_pRoom->sendMsgToPlayer(prealMsg, nMsgType, nSessionID);
	}
}

uint8_t IPrivateRoom::getDiamondNeed( uint8_t nLevel, bool isAA )
{
	if ( !m_pRoom)
	{
		LOGFMTE("room is null , can not return diamond need");
		return -1;
	}
	return m_pRoomMgr->getDiamondNeed(m_pRoom->getRoomType(), nLevel, isAA);
}

void IPrivateRoom::sendRoomPlayersInfo(uint32_t nSessionID)
{
	if (getCoreRoom())
	{
		getCoreRoom()->sendRoomPlayersInfo(nSessionID);
	}
}

void IPrivateRoom::packRoomInfo(Json::Value& jsRoomInfo)
{
	if (m_pRoom)
	{
		m_pRoom->packRoomInfo(jsRoomInfo);
	}

	jsRoomInfo["leftCircle"] = m_nLeftRounds;
	jsRoomInfo["isOpen"] = m_isOpen ? 1 : 0;
	// is waiting vote dismiss room ;
	jsRoomInfo["isWaitingDismiss"] = m_bWaitDismissReply ? 1 : 0;
	int32_t nLeftSec = 0;
	if (m_bWaitDismissReply)
	{
		jsRoomInfo["applyDismissUID"] = m_nApplyDismissUID;
		// find argee idxs ;
		Json::Value jsArgee;
		for (auto& ref : m_vPlayerAgreeDismissRoom)
		{
			auto p = m_pRoom->getPlayerByUID(ref.first);
			if (!p)
			{
				LOGFMTE("%u you are not in room but you reply dissmiss room ", ref.first);
				continue;
			}
			jsArgee[jsArgee.size()] = p->getIdx();
		}

		jsRoomInfo["agreeIdxs"] = jsArgee;

		// caclulate wait time ;
		auto nEsT = time(nullptr) - m_tInvokerTime;
		if (nEsT > TIME_WAIT_REPLY_DISMISS)
		{
			nLeftSec = 1;
		}
		else
		{
			nLeftSec = TIME_WAIT_REPLY_DISMISS - nEsT;
		}
		jsRoomInfo["leftWaitTime"] = nLeftSec;
	}

}

void IPrivateRoom::sendRoomInfo(uint32_t nSessionID)
{
	// send room info ;
	Json::Value jsMsg;
	packRoomInfo(jsMsg);
	sendMsgToPlayer(jsMsg, MSG_ROOM_INFO, nSessionID);

	// send players info ;
	sendRoomPlayersInfo(nSessionID);
}

bool IPrivateRoom::onPlayerNetStateRefreshed(uint32_t nPlayerID, eNetState nState)
{
	if ( eNet_Offline == nState && isRoomStarted() )
	{
		LOGFMTE( "player uid = %u do offline , but can not let player leave room , room is started room id = %u",nPlayerID,getRoomID() );
		return true;
	}
	return m_pRoom->onPlayerNetStateRefreshed(nPlayerID, nState);
}

bool IPrivateRoom::onPlayerSetNewSessionID(uint32_t nPlayerID, uint32_t nSessinID)
{
	return m_pRoom->onPlayerSetNewSessionID(nPlayerID, nSessinID);
}

// delegate interface 
void IPrivateRoom::onStartGame(IGameRoom* pRoom)
{
	if ( eState_WaitStart == m_nPrivateRoomState)
	{
		m_nPrivateRoomState = eState_Started;
	}
}

bool IPrivateRoom::canStartGame(IGameRoom* pRoom)
{
	return m_nLeftRounds > 0 && m_isOpen;
}

void IPrivateRoom::onGameDidEnd(IGameRoom* pRoom)
{
	// decrease round 
	--m_nLeftRounds;
	
	// consume diamond 
	if ( m_isOneRoundNormalEnd == false )
	{
		m_isOneRoundNormalEnd = true;
		auto nNeedDiamond = getDiamondNeed(m_nRoundLevel, m_isAA);
		if ( m_isAA && m_isForFree == false )  // only aa delay consum diamond , owner pay diamond mode , diamond was consumed when create the room ;
		{
			auto nCnt = m_pRoom->getSeatCnt();
			for (uint8_t nIdx = 0; nIdx < nCnt; ++nIdx)
			{
				auto pPlayer = m_pRoom->getPlayerByIdx(nIdx);
				if (!pPlayer)
				{
					//LOGFMTE( "player is null , comuse diamond idx = %u , room id = %u",nIdx , getRoomID() );
					continue;
				}

				Json::Value js;
				js["playerUID"] = pPlayer->getUserUID();
				js["diamond"] = nNeedDiamond;
				js["roomID"] = getRoomID();
				js["reason"] = 0;
				auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
				pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, pPlayer->getUserUID(), eAsync_Consume_Diamond, js);
			}
		}
	}

	// check room over
	if ( 0 == m_nLeftRounds )
	{
		doRoomGameOver(false);
	}
}

void IPrivateRoom::doRoomGameOver(bool isDismissed)
{
	if (eState_RoomOvered == m_nPrivateRoomState )  // avoid opotion  loop invoke this function ;
	{
		LOGFMTE( "already gave over , why invoker again room id = %u",getRoomID() );
		return; 
	}
	// do close room ;
	if ( isRoomStarted() )
	{
		// if we need invoker oom game end ;
		//auto nCurState = m_pRoom->getCurState()->getStateID();
		//if (eRoomSate_WaitReady != nCurState && eRoomState_GameEnd != nCurState && isDismissed )
		//{
		//	m_pRoom->onGameEnd();
		//}

		// prepare game over bills 
		doSendRoomGameOverInfoToClient(isDismissed);
	}

	// give back room card 
	bool isAlreadyComsumedDiamond = ( m_isAA == false && m_isForFree == false );
	if ( isDismissed && isAlreadyComsumedDiamond && m_isOneRoundNormalEnd == false )
	{
		Json::Value jsReq;
		jsReq["targetUID"] = m_nOwnerUID;
		jsReq["diamond"] = getDiamondNeed(m_nRoundLevel, m_isAA);
		jsReq["roomID"] = getRoomID();
		jsReq["reason"] = 1;
		auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, m_nOwnerUID, eAsync_GiveBackDiamond, jsReq);
		LOGFMTD( "room id = %u dissmiss give back uid = %u diamond = %u",getRoomID(),m_nOwnerUID,jsReq["diamond"].asUInt() );
	}

	m_nPrivateRoomState = eState_RoomOvered;
	// delete self
	m_pRoomMgr->deleteRoom(getRoomID());
}

GameRoom* IPrivateRoom::getCoreRoom()
{
	return m_pRoom;
}

uint16_t IPrivateRoom::getRoomPlayerCnt()
{
	uint16_t nCnt = 0;
	auto nSeatCnt = getCoreRoom()->getSeatCnt();
	for ( auto nPlayerIdx = 0; nPlayerIdx < nSeatCnt; ++nPlayerIdx)
	{
		auto p = getCoreRoom()->getPlayerByIdx(nPlayerIdx);
		if (p)
		{
			++nCnt;
		}
	}
	return nCnt;
}

bool IPrivateRoom::isRoomStarted()
{
	return m_nPrivateRoomState == eState_Started;
}