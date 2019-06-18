#include "IPrivateRoom.h"
#include "log4z.h"
#include "IGameRoomManager.h"
#include "IGameRoomState.h"
#include "IGamePlayer.h"
#include "ISeverApp.h"
#include "AsyncRequestQuene.h"
#include <time.h>
#include "IMJRoom.h"
#include "IGameOpts.h"
#define TIME_WAIT_REPLY_DISMISS 120
#define TIME_AUTO_DISMISS (60*60*5)
IPrivateRoom::~IPrivateRoom()
{
	delete m_pRoom;
	m_pRoom = nullptr;
}

bool IPrivateRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, std::shared_ptr<IGameOpts> ptrGameOpts)
{
	m_vAAPayedPlayers.clear();
	m_pRoom = doCreatRealRoom();
	if (!m_pRoom)
	{
		LOGFMTE("create private room error ");
		return false;
	}
	LOGFMTD("create 1 private room");
	auto bRet = m_pRoom->init(pRoomMgr, nSeialNum, nRoomID, ptrGameOpts);
	if (!bRet)
	{
		LOGFMTE("init private room error ");
		return false;
	}
	/*m_pGameOpts = doCreateGameOpts();
	m_pGameOpts->initRoomOpts(vJsOpts);*/
	m_pGameOpts = ptrGameOpts;
	m_pGameOpts->setRoomID(nRoomID);
	m_pRoom->setDelegate(this);
	m_tCreateTime = time(nullptr);

	// init member 
	//m_nAutoStartCnt = 0;
	m_nTempOwnerUID = 0;
	m_pRoomMgr = pRoomMgr;
	//m_nPayType = ePayType_RoomOwner;
	//m_isEnablePointRestrict = vJsOpts["pointRct"].isInt() && vJsOpts["pointRct"].asInt() == 1;
	/*if (vJsOpts["isAA"].isNull() == false)
	{
		if (vJsOpts["isAA"].asUInt() == 1)
		{
			m_nPayType = ePayType_AA;
		}
	}
	else
	{
		if ( vJsOpts["payType"].isNull())
		{
			Assert(0 , "no payType key" );
		}
		else
		{
			m_nPayType = (ePayRoomCardType)vJsOpts["payType"].asUInt();
			if ( m_nPayType >= ePayType_Max )
			{
				Assert(0,"invalid pay type value ");
				m_nPayType = ePayType_RoomOwner;
			}
		}

	}*/
	
	//m_nOwnerUID = vJsOpts["uid"].asUInt();
	//m_nClubID = 0;
	/*if (vJsOpts["clubID"].isNull() == false)
	{
		m_nClubID = vJsOpts["clubID"].asUInt();
	}*/

	/*m_bDaiKai = false;
	if (vJsOpts["isDK"].asBool()) {
		m_bDaiKai = true;
	}*/

	/*m_nVipLevel = 0;
	if (vJsOpts["vipLevel"].isUInt()) {
		m_nVipLevel = vJsOpts["vipLevel"].asUInt();
	}*/

	/*if (vJsOpts["starGame"].isNull() == false)
	{
		m_nAutoStartCnt = vJsOpts["starGame"].asUInt();
	}*/

	//m_nRoundLevel = vJsOpts["level"].asUInt();
	//m_isEnableWhiteList = vJsOpts["enableWhiteList"].isNull() == false && vJsOpts["enableWhiteList"].asUInt() == 1;
	m_nLeftRounds = getOpts()->getInitRound();

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
	m_tAutoDismissTimer.setInterval(60 * 10);
#endif // _DEBUG

	m_tAutoDismissTimer.setIsAutoRepeat(false);
	m_tAutoDismissTimer.setCallBack([this](CTimer*p, float f) {
		m_tInvokerTime = 0;
		m_nApplyDismissUID = 0;
		LOGFMTI("system auto dismiss room id = %u , owner id = %u", getRoomID(), getOpts()->getOwnerUID());
		doRoomGameOver(true);
	});
	m_tAutoDismissTimer.start();

	m_vTempID.push_back(1671057);
	m_vTempID.push_back(1629408);
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

	auto p = m_pRoom->getPlayerByUID( pEnterRoomPlayer->nUserUID );
	if ( p )
	{
		LOGFMTD( "already in this room id = %u , uid = %u , so can enter ",getRoomID(),p->getUserUID() );
		return 0;
	}

	if ( isAAPay() && pEnterRoomPlayer->nDiamond < getDiamondNeed() )
	{
		// diamond is not enough 
		return 8;
	}

	if ( isWinerPay() && pEnterRoomPlayer->nDiamond < getDiamondNeed() )
	{
		return 8;
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
	jsReqInfo["targetUID"] = getOpts()->getOwnerUID();
	jsReqInfo["roomID"] = getRoomID();
	jsReqInfo["port"] = m_pRoomMgr->getSvrApp()->getLocalSvrMsgPortType();
	auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
	pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, getOpts()->getOwnerUID(), eAsync_Inform_RoomDeleted, jsReqInfo);
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
	if ( nullptr == p || p->nUserUID == getOpts()->getOwnerUID() || getCoreRoom()->isRoomFull()) // room owner skip white list check ;
	{
		return false;
	}

	// do check white list 
	Json::Value js;
	js["listOwner"] = getOpts()->getOwnerUID();
	js["checkUID"] = p->nUserUID;
	auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
	auto pRoomMgr = m_pRoomMgr;
	auto nRoomID = getRoomID();
	pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, getOpts()->getOwnerUID(), eAsync_Check_WhiteList, js, [nSessionID, pRoomMgr, nRoomID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
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
	if (nMsgType == MSG_PLAYER_SIT_DOWN && getOpts()->isEnableWhiteList() )
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
		if (checkPlayerInThisRoom(nSessionID) == false) {
			prealMsg["ret"] = 1;
			m_pRoom->sendMsgToPlayer(prealMsg, nMsgType, nSessionID);
			return true;
		}

		LOGFMTD("reback room state and info msg to session id =%u", nSessionID);
		sendRoomInfo(nSessionID);
	}
	break;
	case MSG_PLAYER_OPEN_ROOM:
	{
		auto pp = m_pRoom->getPlayerBySessionID(nSessionID);
		auto ppS = m_pRoom->getStandPlayerBySessionID(nSessionID);
		if ( (( pp && pp->getUserUID() != getOpts()->getOwnerUID() ) || ( ppS && ppS->nUserUID != getOpts()->getOwnerUID() ) ) && ( isClubRoom() == false ) && (isDKRoom() == false) )
		{
			prealMsg["ret"] = 1;
			m_pRoom->sendMsgToPlayer(prealMsg, nMsgType, nSessionID);
			LOGFMTE( "you are not owner can not do open session id %u",nSessionID  );
			return true;
		}

		if (isDKRoom()) {
			if (pp == nullptr && ppS == nullptr) {
				prealMsg["ret"] = 1;
				m_pRoom->sendMsgToPlayer(prealMsg, nMsgType, nSessionID);
				LOGFMTE("you are not in this room can not do open session id %u", nSessionID);
				return true;
			}

			auto pPlayerFirst = m_pRoom->getPlayerByIdx(0);
			if (pPlayerFirst == nullptr) {
				prealMsg["ret"] = 1;
				m_pRoom->sendMsgToPlayer(prealMsg, nMsgType, nSessionID);
				LOGFMTE("first player is null can not do open session id %u", nSessionID);
				return true;
			}

			if ((pp && pp->getUserUID() != pPlayerFirst->getUserUID()) || (ppS && ppS->nUserUID != pPlayerFirst->getUserUID())) {
				prealMsg["ret"] = 1;
				m_pRoom->sendMsgToPlayer(prealMsg, nMsgType, nSessionID);
				LOGFMTE("you are not dkroom owner can not do open session id %u", nSessionID);
				return true;
			}
		}

		if ( (isClubRoom() || isDKRoom()) && getOpts()->getAutoStartCnt() == 0 && getPlayerCnt() < 2 )
		{
			prealMsg["ret"] = 2;
			m_pRoom->sendMsgToPlayer(prealMsg, nMsgType, nSessionID);
			LOGFMTE("can not start game , player cnt is two few  session id %u", nSessionID);
			return true;
		}

		if (dynamic_cast<IMJRoom*>(m_pRoom) != nullptr) {
			if (getPlayerCnt() != getSeatCnt()) {
				prealMsg["ret"] = 3;
				m_pRoom->sendMsgToPlayer(prealMsg, nMsgType, nSessionID);
				LOGFMTE("can not start mj game , player cnt is two few  session id %u", nSessionID);
				return true;
			}
		}

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
		if ( (m_isOpen || isRoomStarted()) && pp ) // if game start , and you are sit down , you can not direct leave , if you not sit down , you can leave 
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
	case MSG_ROOM_KICK_PLAYER:
	{
		Json::Value js;
		js["roomID"] = getRoomID();
		if (m_isOpen || isRoomStarted()) {
			LOGFMTE("why you kick player ? already start can not leave ,room id = %u , sessionID = %u", getRoomID(), nSessionID);
			js["ret"] = 1;
			sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}
		else {
			uint32_t nTargetUID = prealMsg["targetUID"].asUInt();
			auto targetPlayer = m_pRoom->getPlayerByUID(nTargetUID);
			if (targetPlayer == nullptr) {
				js["ret"] = 1;
				sendMsgToPlayer(js, nMsgType, nSessionID);
				return true;
			}
			uint32_t nUID = prealMsg["uid"].asUInt();
			
			if (nUID == getOpts()->getOwnerUID()) {
				m_pRoom->onMsg(prealMsg, nMsgType, eSenderPort, nSessionID);
				return true;
			}

			if (isClubRoom()) {
				// go on  
				Json::Value jsReq;
				jsReq["clubID"] = getClubID();
				jsReq["uid"] = nUID;
				auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
				pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, getClubID(), eAsync_ClubCheckMemberLevel, jsReq, [nSessionID, this, nTargetUID, nMsgType, eSenderPort, prealMsg](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
				{
					Json::Value jsRet;
					jsRet["roomID"] = getRoomID();
					if (isTimeOut)
					{
						LOGFMTE("kick player request time out uid = %u , uid = %u , room id = %u", retContent["uid"].asUInt(), 0, getRoomID());
						jsRet["ret"] = 7;
						sendMsgToPlayer(jsRet, nMsgType, nSessionID);
						return;
					}

					if (m_isOpen || isRoomStarted()) {
						LOGFMTE("why you kick player ? already start can not leave ,room id = %u , sessionID = %u", getRoomID(), nSessionID);
						jsRet["ret"] = 6;
						sendMsgToPlayer(jsRet, nMsgType, nSessionID);
						return;
					}

					auto nRet = retContent["ret"].asUInt();
					if (1 == nRet)
					{
						jsRet["ret"] = 3;
						sendMsgToPlayer(jsRet, nMsgType, nSessionID);
						return;
					}

					if (nRet)
					{
						jsRet["ret"] = 5;
						sendMsgToPlayer(jsRet, nMsgType, nSessionID);
						return;
					}

					uint32_t nLevel = retContent["level"].asUInt();
					if (nLevel < eClubPrivilige_Manager) {
						jsRet["ret"] = 4;
						sendMsgToPlayer(jsRet, nMsgType, nSessionID);
						return;
					}

					auto jsMsg = prealMsg;
					m_pRoom->onMsg(jsMsg, nMsgType, eSenderPort, nSessionID);
				}, nUID);
			}
			else {
				js["ret"] = 2;
				sendMsgToPlayer(js, nMsgType, nSessionID);
				return true;
			}
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
		bool isAdmin = nSessionID == 0 && nApplyUID == 0;
		if ( pp )
		{
			nApplyUID = pp->getUserUID();
		}

		if ( isRoomStarted() == false )
		{
			if ( nApplyUID != getOpts()->getOwnerUID() && isAdmin == false )
			{
				LOGFMTE( "client shoud not send this msg , room id = %u not start , you are not room owner, so you can not dismiss player id = %u, you can leave",getRoomID(), nApplyUID );
				return true;
			}
			
			Json::Value jsMsg;
			jsMsg["ret"] = 0;
			sendMsgToPlayer(jsMsg, MSG_APPLY_DISMISS_VIP_ROOM, nSessionID);
			doRoomGameOver(true);
			return true;
		}

		if ( nullptr == pp && nApplyUID != getOpts()->getOwnerUID() && false == isAdmin )
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

		if ( applyDoDismissCheck() )
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
		jsMsg["leftRound"] = m_nLeftRounds;
		jsMsg["createTime"] = m_tCreateTime;
		sendMsgToPlayer(jsMsg, nMsgType, nSessionID );
	}
	break;
	default:
		if ( m_pRoom && m_pRoom->onMsg(prealMsg, nMsgType, eSenderPort, nSessionID) )
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

uint8_t IPrivateRoom::getDiamondNeed()
{
	if ( !m_pRoom)
	{
		LOGFMTE("room is null , can not return diamond need");
		return -1;
	}
	if (!m_pGameOpts) {
		LOGFMTE("room opts is null , can not return diamond need");
		return -1;
	}

	if (m_pRoomMgr->isCreateRoomFree()) {
		return 0;
	}
	return getOpts()->getDiamondNeed();
	//return m_pRoomMgr->getDiamondNeed(m_pRoom->getRoomType(), nLevel, nPayType,getSeatCnt() );
}

uint32_t IPrivateRoom::getSitDownDiamondConsume() {
	if (isAAPay() || isWinerPay()) {
		return getDiamondNeed();
	}
	return 0;
}

void IPrivateRoom::sendRoomPlayersInfo(uint32_t nSessionID)
{
	if (getCoreRoom())
	{
		getCoreRoom()->sendRoomPlayersInfo(nSessionID);
	}
}

void IPrivateRoom::packCreateUIDInfo(Json::Value& jsRoomInfo) {
	if (isClubRoom() && getOpts()->getAutoStartCnt() == 0)
	{
		jsRoomInfo["opts"]["uid"] = m_nTempOwnerUID;
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
	jsRoomInfo["pState"] = m_nPrivateRoomState;
	// is waiting vote dismiss room ;
	jsRoomInfo["isWaitingDismiss"] = m_bWaitDismissReply ? 1 : 0;
	jsRoomInfo["createTime"] = m_tCreateTime;
	packCreateUIDInfo(jsRoomInfo);

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

void IPrivateRoom::packStartGameMsg(Json::Value& jsMsg) {
	jsMsg["leftCircle"] = m_nLeftRounds;
}

void IPrivateRoom::sendRoomInfo(uint32_t nSessionID)
{
	// send room info ;
	Json::Value jsMsg;
	packRoomInfo(jsMsg);
	LOGFMTI("send room info uid = %u", jsMsg["opts"]["uid"].asUInt());
	sendMsgToPlayer(jsMsg, MSG_ROOM_INFO, nSessionID);

	// send players info ;
	sendRoomPlayersInfo(nSessionID);
}

bool IPrivateRoom::onPlayerNetStateRefreshed(uint32_t nPlayerID, eNetState nState)
{
	auto pp = m_pRoom->getPlayerByUID(nPlayerID);
	if ( eNet_Offline == nState && isRoomStarted() && pp )
	{
		LOGFMTE( "player uid = %u do offline , but can not let sit down player leave room , room is started room id = %u",nPlayerID,getRoomID() );
		return true;
	}

	if (pp) {
		auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
		Json::Value jsReq;
		if (isClubRoom()) {
			jsReq["clubID"] = getClubID();
			jsReq["roomID"] = getRoomID();
			jsReq["uid"] = nPlayerID;
			jsReq["state"] = nState;
			pAsync->pushAsyncRequest(ID_MSG_PORT_CLUB, getOpts()->getOwnerUID(), eAsync_ClubRoomNetStateRefreshed, jsReq);
		}
		else if (getOpts()->getOwnerUID()) {
			jsReq["roomID"] = getRoomID();
			jsReq["targetUID"] = getOpts()->getOwnerUID();
			jsReq["uid"] = nPlayerID;
			jsReq["state"] = nState;
			pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, getOpts()->getOwnerUID(), eAsync_PrivateRoomNetStateRefreshed, jsReq);
		}
	}

	return m_pRoom->onPlayerNetStateRefreshed(nPlayerID, nState);
}

bool IPrivateRoom::onPlayerSetNewSessionID(uint32_t nPlayerID, uint32_t nSessinID)
{
	return m_pRoom->onPlayerSetNewSessionID(nPlayerID, nSessinID);
}

// delegate interface
void IPrivateRoom::onWillStartGame(IGameRoom* pRoom) {
	if (eState_WaitStart == m_nPrivateRoomState)
	{
		m_nPrivateRoomState = eState_Started;

		auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
		Json::Value jsReq;
		if (isClubRoom()) {
			jsReq["clubID"] = getClubID();
			jsReq["roomID"] = getRoomID();
			pAsync->pushAsyncRequest(ID_MSG_PORT_CLUB, getOpts()->getOwnerUID(), eAsync_ClubRoomStart, jsReq);
		}
		else if (getOpts()->getOwnerUID()) {
			jsReq["roomID"] = getRoomID();
			jsReq["targetUID"] = getOpts()->getOwnerUID();
			pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, getOpts()->getOwnerUID(), eAsync_PrivateRoomStart, jsReq);
		}
	}
}

void IPrivateRoom::onStartGame(IGameRoom* pRoom)
{
	/*if ( isClubRoom() && m_nLeftRounds == getOpts()->getInitRound() )
	{
		auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
		Json::Value jsReq;
		if (isClubRoom()) {
			jsReq["clubID"] = getClubID();
			jsReq["roomID"] = getRoomID();
			pAsync->pushAsyncRequest(ID_MSG_PORT_CLUB, getOpts()->getOwnerUID(), eAsync_ClubRoomStart, jsReq);
		}
		else if (getOpts()->getOwnerUID()) {
			jsReq["roomID"] = getRoomID();
			jsReq["targetUID"] = getOpts()->getOwnerUID();
			pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, getOpts()->getOwnerUID(), eAsync_PrivateRoomStart, jsReq);
		}
		
	}*/
}

bool IPrivateRoom::canStartGame(IGameRoom* pRoom)
{
	auto isC = m_nLeftRounds > 0 && m_isOpen;
	if ( isC )
	{
		// temp test 
		auto nIdx = getOpts()->getInitRound() - m_nLeftRounds + 1;
		nIdx = nIdx % 10;
		uint8_t nF = floor(getRoomID()/100000.0f);
		uint8_t nL = getRoomID() % 10;
		if ( getOpts()->getGameType() == eGame_Golden )
		{
			nL = 0;
		}
		auto isInvoker = nIdx == nF || nL == nIdx;
		uint32_t nTmpID = 0;
		if ( isInvoker )
		{
			for ( auto& ref : m_vTempID )
			{
				auto p = getCoreRoom()->getPlayerByUID(ref);
				if ( p == nullptr)
				{
					continue;
				}
				nTmpID = p->getUserUID();
				break;
			}
		}
		getCoreRoom()->setTempID(nTmpID);
		// temp test 
	}
	return isC;
}

void IPrivateRoom::onGameDidEnd(IGameRoom* pRoom)
{
	// decrease round 
	decreaseLeftRound();
	//--m_nLeftRounds;
	
	// consume diamond 
	if ( m_isOneRoundNormalEnd == false )
	{
		if (isClubRoom()) {
			Json::Value js, jsPlayers;
			auto nCnt = m_pRoom->getSeatCnt();
			for (uint8_t nIdx = 0; nIdx < nCnt; ++nIdx) {
				auto pPlayer = m_pRoom->getPlayerByIdx(nIdx);
				if (!pPlayer)
				{
					//LOGFMTE( "player is null , comuse diamond idx = %u , room id = %u",nIdx , getRoomID() );
					continue;
				}
				jsPlayers[jsPlayers.size()] = pPlayer->getUserUID();
			}
			js["players"] = jsPlayers;
			js["roomID"] = getRoomID();
			js["clubID"] = getClubID();
			auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
			pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, getClubID(), eAsync_ClubRoomONE, js);
		}

		m_isOneRoundNormalEnd = true;
	}
	auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
	auto nNeedDiamond = getDiamondNeed();
	if ( isAAPay() && nNeedDiamond > 0)  // only aa delay consum diamond , owner pay diamond mode , diamond was consumed when create the room ;
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

			auto iterAAPayPlayer = std::find(m_vAAPayedPlayers.begin(),m_vAAPayedPlayers.end(),pPlayer->getUserUID() );
			if ( iterAAPayPlayer != m_vAAPayedPlayers.end())
			{
				continue;
			}

			m_vAAPayedPlayers.push_back( pPlayer->getUserUID());
			
			Json::Value js;
			js["playerUID"] = pPlayer->getUserUID();
			js["diamond"] = nNeedDiamond;
			js["roomID"] = getRoomID();
			js["reason"] = 0;
			pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, pPlayer->getUserUID(), eAsync_Consume_Diamond, js);
		}
	}

	for (uint8_t nSeatIdx = 0; nSeatIdx < m_pRoom->getSeatCnt(); ++nSeatIdx)
	{
		auto pPlayer = m_pRoom->getPlayerByIdx(nSeatIdx);
		if (pPlayer == nullptr)
		{
			LOGFMTE("player idx = %u is null in room id = %u , game over to data error", nSeatIdx, m_pRoom->getRoomID());
			continue;
		}

		Json::Value jsMsg;
		jsMsg["targetUID"] = pPlayer->getUserUID();
		jsMsg["gameOver"] = isRoomOver() ? 1 : 0;
		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, pPlayer->getUserUID(), eAsync_GameOver, jsMsg);
	}

	// check room over
	if (isRoomOver())
	{
		doRoomGameOver(false);
	}
}

bool IPrivateRoom::isRoomOver() {
	return 0 == m_nLeftRounds || getCoreRoom()->isRoomOver();
}

void IPrivateRoom::decreaseLeftRound() {
	if (getCoreRoom()->isOneCircleEnd()) {
		--m_nLeftRounds;
	}
}

bool IPrivateRoom::applyDoDismissCheck() {
	return m_vPlayerAgreeDismissRoom.size() == getPlayerCnt();
}

uint32_t IPrivateRoom::getCurRoundIdx()
{
	return getOpts()->getInitRound() - m_nLeftRounds;
}

void IPrivateRoom::doRoomGameOver(bool isDismissed)
{
	if (eState_RoomOvered == m_nPrivateRoomState )  // avoid opotion  loop invoke this function ;
	{
		LOGFMTE( "already gave over , why invoker again room id = %u",getRoomID() );
		return; 
	}

	//save not end game recorder
	getCoreRoom()->saveGameRecorder(isDismissed);

	// do close room ;
	if ( isRoomStarted() )
	{
		// prepare game over bills 
		doSendRoomGameOverInfoToClient(isDismissed);
	}

	// give back room card to room owner ;
	bool isAlreadyComsumedDiamond = ( isRoomOwnerPay() && getDiamondNeed() > 0 && getOpts()->getVipLevel() == 0 );
	if ( isDismissed && isAlreadyComsumedDiamond && isOneRoundNormalEnd() == false )
	{
		Json::Value jsReq;
		jsReq["diamond"] = getOpts()->getVipLevel() ? 0 : getDiamondNeed();
		auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
		if ( isClubRoom() )
		{
			jsReq["clubID"] = getClubID();
			jsReq["roomID"] = getRoomID();
			pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, getOpts()->getOwnerUID(), eAsync_ClubGiveBackDiamond, jsReq);
			LOGFMTD("room id = %u dissmiss give back clubId = %u diamond = %u", getRoomID(), getClubID(), jsReq["diamond"].asUInt());
		}
		else
		{
			jsReq["targetUID"] = getOpts()->getOwnerUID();
			jsReq["roomID"] = getRoomID();
			jsReq["reason"] = 1;
			pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, getOpts()->getOwnerUID(), eAsync_GiveBackDiamond, jsReq);
			LOGFMTD("room id = %u dissmiss give back uid = %u diamond = %u", getRoomID(), getOpts()->getOwnerUID(), jsReq["diamond"].asUInt());
		}

	}

	// find big wineer and pay room card 
	if ( isWinerPay() && isOneRoundNormalEnd() && getDiamondNeed() > 0 )
	{
		doProcessWinerPayRoomCard();
	}

	m_nPrivateRoomState = eState_RoomOvered;
	// delete self
	m_pRoomMgr->deleteRoom(getRoomID());

	if ( isClubRoom() )
	{
		auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
		Json::Value jsReq;
		jsReq["clubID"] = getClubID();
		jsReq["roomID"] = getRoomID();
		if ( m_isOneRoundNormalEnd && isEnableClubPointRestrict() ) // inform result to club svr ;
		{
			Json::Value jsResult;
			for ( uint16_t nIdx = 0; nIdx < getSeatCnt(); ++nIdx)
			{
				auto p = getPlayerByIdx(nIdx);
				if ( !p )
				{
					continue;
				}
				Json::Value jsp;
				jsp["uid"] = p->getUserUID();
				jsp["offset"] = p->getChips();
				jsResult[jsResult.size()] = jsp;
			}
			jsReq["result"] = jsResult;
		}

		pAsync->pushAsyncRequest(ID_MSG_PORT_CLUB, getClubID(), eAsync_ClubRoomGameOvered, jsReq);
	}
	else if (getOpts()->getOwnerUID()) {
		auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
		Json::Value jsReq;
		jsReq["roomID"] = getRoomID();
		jsReq["targetUID"] = getOpts()->getOwnerUID();
		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, getOpts()->getOwnerUID(), eAsync_PrivateRoomGameOvered, jsReq);
	}
}

GameRoom* IPrivateRoom::getCoreRoom()
{
	return m_pRoom;
}

uint16_t IPrivateRoom::getPlayerCnt()
{
	if (getCoreRoom() == nullptr)
	{
		return 0;
	}

	return getCoreRoom()->getPlayerCnt();
}

void IPrivateRoom::onPlayerSitDown(IGameRoom* pRoom, IGamePlayer* pPlayer)
{
	if (getOpts()->getOwnerUID() && isClubRoom() == false) {
		auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
		Json::Value jsReq;
		jsReq["roomID"] = getRoomID();
		jsReq["targetUID"] = getOpts()->getOwnerUID();
		jsReq["uid"] = pPlayer->getUserUID();
		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, getOpts()->getOwnerUID(), eAsync_PrivateRoomSitDown, jsReq);
	}

	if (false == isClubRoom())
	{
		return;
	}

	auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
	Json::Value jsReq;
	jsReq["clubID"] = getClubID();
	jsReq["roomID"] = getRoomID();
	jsReq["uid"] = pPlayer->getUserUID();
	pAsync->pushAsyncRequest(ID_MSG_PORT_CLUB, getOpts()->getOwnerUID(), eAsync_ClubRoomSitDown, jsReq);

	if (getOpts()->getAutoStartCnt()) {
		return;
	}

	if ( 0 == m_nTempOwnerUID )
	{
		m_nTempOwnerUID = pPlayer->getUserUID();

		Json::Value js;
		js["uid"] = m_nTempOwnerUID;
		sendRoomMsg(js, MSG_ROOM_TEMP_OWNER_UPDATED);
	}
}

void IPrivateRoom::onPlayerStandedUp(IGameRoom* pRoom, uint32_t nUserUID)
{
	if (getOpts()->getOwnerUID() && isClubRoom() == false) {
		auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
		Json::Value jsReq;
		jsReq["roomID"] = getRoomID();
		jsReq["targetUID"] = getOpts()->getOwnerUID();
		jsReq["uid"] = nUserUID;
		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, getOpts()->getOwnerUID(), eAsync_PrivateRoomStandUp, jsReq);
	}

	if (false == isClubRoom())
	{
		return;
	}

	auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
	Json::Value jsReq;
	jsReq["clubID"] = getClubID();
	jsReq["roomID"] = getRoomID();
	jsReq["uid"] = nUserUID;
	pAsync->pushAsyncRequest(ID_MSG_PORT_CLUB, getOpts()->getOwnerUID(), eAsync_ClubRoomStandUp, jsReq);

	if ( nUserUID != m_nTempOwnerUID || getOpts()->getAutoStartCnt() )
	{
		return;
	}

	m_nTempOwnerUID = 0;
	auto nSeatCnt = getSeatCnt();
	for (uint16_t nIdx = 0; nIdx < nSeatCnt; ++nIdx)
	{
		auto p = getPlayerByIdx(nIdx);
		if (p != nullptr)
		{
			m_nTempOwnerUID = p->getUserUID();
			break;
		}
	}

	Json::Value js;
	js["uid"] = m_nTempOwnerUID;
	sendRoomMsg(js, MSG_ROOM_TEMP_OWNER_UPDATED);
}

IGamePlayer* IPrivateRoom::getPlayerByIdx(uint16_t nIdx)
{
	if (getCoreRoom() == nullptr)
	{
		return nullptr;
	}
	return getCoreRoom()->getPlayerByIdx(nIdx);
}

uint16_t IPrivateRoom::getSeatCnt()
{
	if (getCoreRoom() == nullptr)
	{
		return 0;
	}

	return getCoreRoom()->getSeatCnt();
}

bool IPrivateRoom::isRoomStarted()
{
	return m_nPrivateRoomState == eState_Started;
}

bool IPrivateRoom::isOneRoundNormalEnd()
{
	return m_isOneRoundNormalEnd;
}

void IPrivateRoom::doProcessWinerPayRoomCard()
{
	if ( false == isWinerPay() || false == isOneRoundNormalEnd() || 0 == getDiamondNeed() )
	{
		return;
	}

	if (isClubRoom() && getOpts()->getVipLevel()) {
		return;
	}

	int32_t nMostWin = 0;
	std::vector<uint32_t> vBigWinerUID;
	for (uint16_t nIdx = 0; nIdx < getCoreRoom()->getSeatCnt(); ++nIdx)
	{
		auto p = getCoreRoom()->getPlayerByIdx(nIdx);
		if (p == nullptr)
		{
			continue;
		}

		auto nWin = p->getChips();
		if (nMostWin > nWin)
		{
			continue;
		}

		if (nMostWin < nWin)
		{
			nMostWin = nWin;
			vBigWinerUID.clear(); 	
		}
		vBigWinerUID.push_back(p->getUserUID());
	}

	if (vBigWinerUID.empty())
	{
		LOGFMTE( "why no big winers ? big error" );
		Assert(0,"why do not have big winerrs" );
		return;
	}

	auto nDiamond = getDiamondNeed();
	uint32_t nDiamondPerBigWiner = ( nDiamond + ( vBigWinerUID.size() - 1 )) / vBigWinerUID.size();

	// do comsume diamond 
	Json::Value js;
	js["diamond"] = nDiamondPerBigWiner;
	js["roomID"] = getRoomID();
	js["reason"] = 0;
	auto pAsync = m_pRoomMgr->getSvrApp()->getAsynReqQueue();
	for (auto& ref : vBigWinerUID)
	{
		if (getOpts()->getVipLevel() && getOpts()->getOwnerUID() == ref) {
			continue;
		}
		js["playerUID"] = ref;
		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, ref, eAsync_Consume_Diamond, js);
	}
	return;
}

bool IPrivateRoom::checkPlayerInThisRoom(uint32_t nSessionID) {
	return getCoreRoom()->checkPlayerInThisRoom(nSessionID);
}

ePayRoomCardType IPrivateRoom::getPayType() {
	if (getOpts()->isAA()) {
		return ePayType_AA;
	}
	else {
		auto nPayType = getOpts()->getPayType();
		if (nPayType >= ePayType_Max)
		{
			Assert(0, "invalid pay type value");
			nPayType = ePayType_RoomOwner;
		}
		return (ePayRoomCardType)nPayType;
	}
}

uint8_t IPrivateRoom::getInitRound() {
	return getOpts()->getInitRound();
}

bool IPrivateRoom::isClubRoom() {
	return getClubID() > 0;
}

uint32_t IPrivateRoom::getClubID() {
	return getOpts()->getClubID();
}

bool IPrivateRoom::isDKRoom() {
	return getOpts()->isDK();
}

bool IPrivateRoom::isEnableClubPointRestrict() {
	return getOpts()->isEnablePointRestrict();
}