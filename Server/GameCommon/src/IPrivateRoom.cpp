#include "IPrivateRoom.h"
#include "log4z.h"
#include "IGameRoomManager.h"
#include "IGameRoomState.h"
#include "IGamePlayer.h"
#include "ISeverApp.h"
#include "AsyncRequestQuene.h"
#include <time.h>
#define TIME_WAIT_REPLY_DISMISS 90
IPrivateRoom::~IPrivateRoom()
{
	delete m_pRoom;
	m_pRoom = nullptr;
}

bool IPrivateRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts )
{
	m_pRoom = doCreatRealRoom(vJsOpts);
	if (!m_pRoom)
	{
		LOGFMTE("create private room error ");
		return false;
	}
	LOGFMTD("create 1 private room");
	m_pRoom->setDelegate(this);
	auto bRet = m_pRoom->init(pRoomMgr, nSeialNum, nRoomID, nSeatCnt, vJsOpts);
	if (!bRet)
	{
		LOGFMTE("init private room error ");
		return false;
	}

	// init member 
	m_pRoomMgr = pRoomMgr;
	m_isForFree = vJsOpts["isFree"].asUInt() == 1;
	m_isAA = vJsOpts["isAA"].asUInt() == 1;
	m_nOwnerUID = vJsOpts["createUID"].asUInt();
	m_nRoundLevel = vJsOpts["level"].asUInt();
	m_nLeftRounds = getInitRound(m_nRoundLevel);

	m_bComsumedRoomCards = false;
	m_nPrivateRoomState = eState_WaitStart;
	m_bWaitDismissReply = false;

	m_vPlayerAgreeDismissRoom.clear();
	m_tInvokerTime = 0;
	m_nApplyDismissUID = 0;
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
	m_pRoom->onPlayerEnter(pEnterRoomPlayer);
	return true;
}

uint8_t IPrivateRoom::checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)
{
	if ( isRoomStarted() )
	{
		return false;
	}


	if ( m_isAA )
	{
		if ( m_bComsumedRoomCards == false && pEnterRoomPlayer->nDiamond < getDiamondNeed(m_nRoundLevel,m_isAA) )
		{
			// diamond is not enough 
			return 3;
		}
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
	// tell client closed room ;
	Json::Value jsDoClosed;
	jsDoClosed["roomID"] = getRoomID();
	jsDoClosed["isDismiss"] = m_nApplyDismissUID > 0 ? 1 : 0;
	sendRoomMsg(jsDoClosed, MSG_VIP_ROOM_DO_CLOSED);

	// tell data svr , the room is closed 
	LOGFMTE("must tell data svr a room delte ");
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

bool IPrivateRoom::onMsg( Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID )
{
	switch (nMsgType)
	{
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
		if (pp == nullptr)
		{
			LOGFMTE("pp is null why , you apply dismiss , but , you are not sit in room, session id = %u", nSessionID);
			return true;
		}

		if ( isRoomStarted() == false )
		{
			if ( pp->getUserUID() != m_nOwnerUID )
			{
				LOGFMTE( "client shoud not send this msg , room id = %u not start , you are not room owner, so you can not dismiss player id = %u, you can leave",getRoomID(),pp->getUserUID() );
				return true;
			}
			
			doRoomGameOver(true);
			return true;
		}

		m_vPlayerAgreeDismissRoom[pp->getUserUID()] = 1;
		if (m_bWaitDismissReply)
		{
			LOGFMTE("client should not send this msg ,already waiting reply %u why you go on apply ?", pp->getUserUID() );
			return false;
		}
		m_bWaitDismissReply = true;
		m_tInvokerTime = time(nullptr);
		m_nApplyDismissUID = pp->getUserUID();

		Json::Value jsMsg;
		jsMsg["applyerIdx"] = pp->getIdx();
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

		if ( m_vPlayerAgreeDismissRoom.size() == m_pRoom->getSeatCnt() )
		{
			m_tWaitReplyDismissTimer.canncel();
			m_tWaitReplyDismissTimer.reset();
			doRoomGameOver(true);
		}
	}
	break;
	default:
		if (m_pRoom)
		{
			return m_pRoom->onMsg(prealMsg, nMsgType, eSenderPort, nSessionID);
		}
	}

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

void IPrivateRoom::sendRoomInfo(uint32_t nSessionID)
{
	if (m_pRoom)
	{
		m_pRoom->sendRoomInfo(nSessionID);
	}
	else
	{
		LOGFMTE("private room core is null , can not send detail info");
		return;
	}

	LOGFMTD("send vip room info ext to player session id = %u", nSessionID);
	Json::Value jsMsg;
	jsMsg["leftCircle"] = m_nLeftRounds;
	jsMsg["creatorUID"] = m_nOwnerUID;
	jsMsg["level"] = m_nRoundLevel;
	jsMsg["roomType"] = m_pRoom->getRoomType();
	jsMsg["isAA"] = m_isAA ? 1 : 0;
	// is waiting vote dismiss room ;
	jsMsg["isWaitingDismiss"] = m_bWaitDismissReply ? 1 : 0;
	int32_t nLeftSec = 0;
	if (m_bWaitDismissReply)
	{
		jsMsg["applyDismissUID"] = m_nApplyDismissUID;
		// find argee idxs ;
		Json::Value jsArgee;
		for (auto& ref : m_vPlayerAgreeDismissRoom )
		{
			auto p = m_pRoom->getPlayerByUID(ref.first);
			if (!p)
			{
				LOGFMTE("%u you are not in room but you reply dissmiss room ", ref.first);
				continue;
			}
			jsArgee[jsArgee.size()] = p->getIdx();
		}

		jsMsg["agreeIdxs"] = jsArgee;

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
	}

	jsMsg["leftWaitTime"] = nLeftSec;

	sendMsgToPlayer(jsMsg, MSG_VIP_ROOM_INFO_EXT, nSessionID);
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
	return m_nLeftRounds > 0;
}

void IPrivateRoom::onGameDidEnd(IGameRoom* pRoom)
{
	// decrease round 
	--m_nLeftRounds;
	
	// consume diamond 
	if ( m_bComsumedRoomCards == false && m_isForFree == false )
	{
		m_bComsumedRoomCards = true;
		auto nNeedDiamond = getDiamondNeed(m_nRoundLevel, m_isAA);
		if ( m_isAA )
		{
			auto nCnt = m_pRoom->getSeatCnt();
			for (uint8_t nIdx = 0; nIdx < nCnt; ++nIdx)
			{
				auto pPlayer = m_pRoom->getPlayerByIdx(nIdx);
				if (!pPlayer)
				{
					LOGFMTE( "player is null , comuse diamond idx = %u , room id = %u",nIdx , getRoomID() );
					continue;
				}

				Json::Value js;
				js["playerUID"] = pPlayer->getUserUID();
				js["diamond"] = nNeedDiamond;
				js["roomID"] = getRoomID();
				auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
				pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, pPlayer->getUserUID(), eAsync_Consume_Diamond, js);
			}
		}
		else
		{
			Json::Value js;
			js["playerUID"] = m_nOwnerUID;
			js["diamond"] = nNeedDiamond;
			js["roomID"] = getRoomID();
			auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
			pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, m_nOwnerUID, eAsync_Consume_Diamond, js);
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
		auto nCurState = m_pRoom->getCurState()->getStateID();
		if (eRoomSate_WaitReady != nCurState && eRoomState_GameEnd != nCurState)
		{
			m_pRoom->onGameEnd();
		}

		// prepare game over bills 
		doSendRoomGameOverInfoToClient(isDismissed);
	}

	m_nPrivateRoomState = eState_RoomOvered;
	// delete self
	m_pRoomMgr->deleteRoom(getRoomID());
}

bool IPrivateRoom::isRoomStarted()
{
	return m_nPrivateRoomState == eState_Started;
}