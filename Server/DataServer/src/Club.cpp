#include "Club.h"
#include "ClubManager.h"
#include "log4z.h"
#include <ctime>
#include "DataServerApp.h"
#include "Player.h"
#include "PlayerMail.h"
#include "PlayerBaseData.h"
#include "AsyncRequestQuene.h"
#include "MailModule.h"
#include <algorithm>
#define MAX_EMPTY_ROOM_CNT 2 
Club::~Club()
{
	// remove member 
	for (auto& ref : m_vMembers)
	{
		delete ref.second;
		ref.second = nullptr;
	}
	m_vMembers.clear();

	// remove event ;
	for (auto ref : m_vEvents)
	{
		delete ref.second;
		ref.second = nullptr;
	}
	m_vEvents.clear();

	for (auto& ref : m_vInvitations)
	{
		delete ref;
		ref = nullptr;
	}
	m_vInvitations.clear();
}

bool Club::init( ClubManager * pClubMgr,uint32_t nClubID ,std::string& strClubName, Json::Value& jsCreateRoomOpts, uint32_t nCapacity , uint16_t nMaxMgrCnt, uint8_t nState, uint8_t nCPRoomState, uint8_t nAutoJoin, uint32_t nDiamond,std::string strNotice)
{
	m_pMgr = pClubMgr;
	m_jsCreateRoomOpts = jsCreateRoomOpts;
	m_nMaxEventID = 0;
	m_nClubID = nClubID;
	m_nCapacity = nCapacity;
	m_nMaxMgrCnt = nMaxMgrCnt;
	m_strName = strClubName;
	m_nState = nState;
	m_nCreatePRoomState = nCPRoomState;
	m_nAutoJoin = nAutoJoin;
	m_nDiamond = nDiamond;
	m_isCreatingRoom = false;
	m_fDelayTryCreateRoom = 0;
	m_isClubInfoDirty = false;
	m_strNotice = strNotice;
	m_nMaxRoomIdx = 0;
	m_isFinishReadEvent = false;
	m_isFinishReadMembers = false;
	return true;
}

bool Club::onMsg( Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID )
{
	switch ( nMsgType )
	{
	case MSG_CLUB_PLAYER_RTI:
	{
		Json::Value js;
		if (isHaveMemeber(nTargetID) == false) {
			js["ret"] = 1;
			sendMsg(js, nMsgType, nTargetID, nSenderID);
			return true;
		}

		auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nTargetID);
		if (pPlayer == nullptr) {
			js["ret"] = 2;
			sendMsg(js, nMsgType, nTargetID, nSenderID);
			return true;
		}

		bool bState = prealMsg["state"].asBool();
		if (bState == isInIRT(nTargetID)) {
			js["ret"] = 3;
			sendMsg(js, nMsgType, nTargetID, nSenderID);
			return true;
		}

		if (bState) {
			joinIRT(nTargetID);
		}
		else {
			removeFromIRT(nTargetID);
		}
		js["ret"] = 0;
		sendMsg(js, nMsgType, nTargetID, nSenderID);
	}
	break;
	case MSG_CLUB_PLAYER_APPLY_ENTER_ROOM:
	{
		Json::Value js;
		uint32_t nUserID = prealMsg["uid"].asUInt();
		if (isHaveMemeber(nUserID) == false) {
			js["ret"] = 1;
			sendMsg(js, nMsgType, nTargetID, nSenderID);
			return true;
		}

		if (m_vEmptyRooms.empty()) {
			js["ret"] = 2;
			sendMsg(js, nMsgType, nTargetID, nSenderID);
			return true;
		}

		uint8_t nDeficiency = 0;
		auto itEnter = m_vEmptyRooms.begin();
		for (auto it = m_vEmptyRooms.begin(); it != m_vEmptyRooms.end(); it++) {
			auto tDeficiency = it->getDeficiency();
			if (tDeficiency) {
				if (nDeficiency) {
					if (tDeficiency < nDeficiency) {
						itEnter = it;
						nDeficiency = tDeficiency;
					}
				}
				else {
					itEnter = it;
					nDeficiency = tDeficiency;
				}
			}
		}

		if (nDeficiency == 0) {
			js["ret"] = 2;
			sendMsg(js, nMsgType, nTargetID, nSenderID);
			return true;
		}

		js["ret"] = 0;
		js["roomID"] = itEnter->nRoomID;
		sendMsg(js, nMsgType, nTargetID, nSenderID);
	}
	break;
	case MSG_CLUB_CREATE_PRIVATE_ROOM:
	{
		Json::Value js;
		if (m_nCreatePRoomState == 0) {
			js["ret"] = 7;
			sendMsg(js, nMsgType, nTargetID, nSenderID);
			return true;
		}

		auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nTargetID);
		if (pPlayer == nullptr) {
			js["ret"] = 1;
			sendMsg(js, nMsgType, nTargetID, nSenderID);
			return true;
		}

		auto pMember = getMember(nTargetID);
		if (pMember == nullptr || pMember->nPlayTime == 1) {
			js["ret"] = 6;
			sendMsg(js, nMsgType, nTargetID, nSenderID);
			return true;
		}

		m_isCreatingRoom = true;
		Json::Value jsCreate;
		jsCreate = prealMsg["create"];
		auto nRoomType = jsCreate["gameType"].asUInt();
		jsCreate["uid"] = pPlayer->getUserUID();
		jsCreate["clubID"] = getClubID();
		//m_jsCreateRoomOpts["diamond"] = getDiamond();
		jsCreate["clubName"] = getName();
		auto nPort = getTargetPortByGameType(nRoomType);
		if (nPort >= ID_MSG_PORT_MAX)
		{
			m_isCreatingRoom = false;
			LOGFMTE("targe port error can not create room game type = %u", nRoomType);
			js["ret"] = 2;
			sendMsg(js, nMsgType, nTargetID, nSenderID);
			return true;
		}

		if (ID_MSG_PORT_FXMJ == nPort) {
			if (isCreatorReady() == false) {
				m_isCreatingRoom = false;
				LOGFMTE("club %u creator is not ready, delay try to create private room", getClubID());
				js["ret"] = 11;
				sendMsg(js, nMsgType, nTargetID, nSenderID);
				return true;
			}
			jsCreate["diamond"] = getCreatorDiamond();
		}
		else {
			jsCreate["diamond"] = getDiamond();
		}

		auto asyq = m_pMgr->getSvrApp()->getAsynReqQueue();
		jsCreate["roomIdx"] = ++m_nMaxRoomIdx;
		if (m_nMaxRoomIdx >= 99)
		{
			m_nMaxRoomIdx = 0;
		}

		uint8_t nSeatCnt = jsCreate["seatCnt"].asUInt();
		asyq->pushAsyncRequest(nPort, 0, eAsync_ClubCreateRoom, jsCreate, [this, nMsgType, nSenderID, nTargetID, nPort, nSeatCnt](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut) {
			m_isCreatingRoom = false;
			Json::Value js;
			if (isTimeOut)
			{
				js["ret"] = 3;
				sendMsg(js, nMsgType, nTargetID, nSenderID);
				return;
			}

			uint8_t nRet = retContent["ret"].asUInt();
			if (0 == nRet)
			{
				uint32_t nRoomID = retContent["roomID"].asUInt();
				js["roomID"] = nRoomID;
				if (checkDiamondEnough(retContent["diamondFee"].asUInt(), nRoomID)) {
					onCreateEmptyRoom(nRoomID, retContent["diamondFee"].asInt(), retContent["roomIdx"].asInt(), nSeatCnt, true);
					js["ret"] = 0;
				}
				else {
					m_pMgr->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(nPort, 0, eAsync_ClubDismissRoom, js, nRoomID);
					js["ret"] = 11;
				}

				//onCreateEmptyRoom(retContent["roomID"].asUInt(), retContent["diamondFee"].asInt(), retContent["roomIdx"].asInt(), true);
				//js["roomID"] = retContent["roomID"].asUInt();
				js["clubID"] = getClubID();
				//js["ret"] = 0;
				sendMsg(js, nMsgType, nTargetID, nSenderID);
				return;
			}

			// ret:  1 ,diamond is not enough ,  2  admin stoped create room , 3  room ptr is null, 4 room id run out.
			LOGFMTE("club id = %u create room ret = %u", getClubID(), nRet);
			js["ret"] = 10 + nRet;
			sendMsg(js, nMsgType, nTargetID, nSenderID);

			/*if (1 == nRet)
			{
				m_isLackDiamond = true;
				return;
			}*/

			//m_fDelayTryCreateRoom = 60 * 8; // delay create room ; error so , try later ;
		});
		return true;
	}
	break;
	case MSG_CLUB_APPLY_JOIN:
	{
		uint8_t nRet = 0;
		uint32_t nUID = 0;
		do
		{
			if (m_vMembers.size() >= m_nCapacity)
			{
				nRet = 3;
				break;
			}

			auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nTargetID);
			if (pPlayer == nullptr)
			{
				nRet = 4;
				break;
			}

			nUID = pPlayer->getUserUID();
			if ( isHaveMemeber(nUID) )
			{
				nRet = 1;
				break;
			}

			for (auto ref : m_vEvents)
			{
				if (ref.second->nState != eEventState_WaitProcesse || eClubEvent_ApplyJoin != ref.second->nEventType )
				{
					continue;
				}

				uint32_t uid = ref.second->jsEventDetail["uid"].asUInt();
				if ( nUID == uid)
				{
					nRet = 2;
					break;
				}
			}

		} while ( 0 );
		
		Json::Value js;
		js["ret"] = nRet;
		sendMsg(js, nMsgType, nTargetID, nSenderID);
		if ( nRet )
		{
			LOGFMTE( "can not apply uid = %u , ret = %u" , nUID, nRet );
			break;
		}

		auto p = new stClubEvent();
		p->nEventID = ++m_nMaxEventID;
		p->nEventType = eClubEvent_ApplyJoin;
		p->nState = eEventState_WaitProcesse;
		p->nTime = time(nullptr);
		p->jsEventDetail["uid"] = nUID;
		addEvent(p);
	}
	break;
	case MSG_CLUB_KICK_PLAYER:
	{
		uint8_t nRet = 0;
		uint32_t nMgrUID = 0;
		uint32_t nKickUID = prealMsg["kickUID"].asUInt();
		do
		{
			if ( isHaveMemeber(nKickUID) == false )
			{
				nRet = 1;
				break;
			}

			auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nTargetID);
			if (pPlayer == nullptr)
			{
				nRet = 4;
				break;
			}

			nMgrUID = pPlayer->getUserUID();
			auto pMem = getMember( nMgrUID );
			auto pKickMem = getMember( nKickUID );
			if ( pMem == nullptr || pMem->ePrivilige <= pKickMem->ePrivilige )
			{
				nRet = 2;
				break;
			}

		} while (0);

		prealMsg["ret"] = nRet;
		sendMsg(prealMsg, nMsgType, nTargetID, nSenderID);
		if ( nRet == 0 )
		{
			deleteMember(nKickUID);

			auto p = new stClubEvent();
			p->nEventID = ++m_nMaxEventID;
			p->nEventType = eClubEvent_Kick;
			p->nState = eEventState_Processed;
			p->nTime = time(nullptr);
			p->jsEventDetail["uid"] = nKickUID;
			p->jsEventDetail["mgrUID"] = nMgrUID;
			addEvent(p);

			// infom player ;
			Json::Value js;
			js["clubID"] = getClubID();
			js["clubName"] = m_strName;
			js["mgrID"] = nMgrUID;
			postMail(nKickUID, eMail_ClubBeKick, js, eMailState_SysProcessed);
		}
	}
	break;
	case MSG_CLUB_PLAYER_APPLY_LEAVE:
	{
		uint8_t nRet = 0;
		uint32_t nUID = 0;
		do
		{
			auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nTargetID);
			if (pPlayer == nullptr)
			{
				nRet = 4;
				break;
			}

			nUID = pPlayer->getUserUID();
			if (isHaveMemeber(nUID))
			{
				nRet = 1;
				break;
			}

			for (auto ref : m_vEvents)
			{
				if (ref.second->nState != eEventState_WaitProcesse || eClubEvent_ApplyLeave != ref.second->nEventType)
				{
					continue;
				}

				uint32_t uid = ref.second->jsEventDetail["uid"].asUInt();
				if (nUID == uid)
				{
					nRet = 2;
					break;
				}
			}

		} while (0);

		Json::Value js;
		js["ret"] = nRet;
		sendMsg(js, nMsgType, nTargetID, nSenderID);
		if (nRet)
		{
			LOGFMTE("can not apply leave uid = %u , ret = %u", nUID, nRet);
			break;
		}

		auto p = new stClubEvent();
		p->nEventID = ++m_nMaxEventID;
		p->nEventType = eClubEvent_ApplyLeave;
		p->nState = eEventState_WaitProcesse;
		p->nTime = time(nullptr);
		p->jsEventDetail["uid"] = nUID;
		addEvent(p);
	}
	break;
	case MSG_CLUB_PLAYER_LEAVE:
	{
		uint32_t nRet = 0;
		uint32_t nLeaveUID = 0;
		do
		{
			auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nTargetID);
			if (pPlayer == nullptr)
			{
				nRet = 4;
				break;
			}

			nLeaveUID = pPlayer->getUserUID();
			if ( false == isHaveMemeber(nLeaveUID) )
			{
				nRet = 1;
				break;
			}

		} while ( 0 );

		prealMsg["ret"] = nRet;
		sendMsg(prealMsg, nMsgType, nTargetID, nSenderID);
		if ( 0 == nRet )
		{
			deleteMember( nLeaveUID );

			auto p = new stClubEvent();
			p->nEventID = ++m_nMaxEventID;
			p->nEventType = eClubEvent_Leave;
			p->nState = eEventState_Processed;
			p->nTime = time(nullptr);
			p->jsEventDetail["uid"] = nLeaveUID;
			addEvent(p);
		}
	}
	break;
	case MSG_CLUB_SET_ROOM_OPTS:
	{
		uint8_t nRet = 0;
		do
		{
			auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nTargetID);
			if (pPlayer == nullptr)
			{
				nRet = 4;
				break;
			}
			auto pmem = getMember( pPlayer->getUserUID() );
			if ( pmem == nullptr || pmem->ePrivilige != eClubPrivilige_Creator)
			{
				nRet = 1;
				break;
			}
		} while ( 0 );

		prealMsg["ret"] = nRet;
		sendMsg(prealMsg, nMsgType, nTargetID, nSenderID);
		if (nRet)
		{
			break;
		}
		LOGFMTD( "club id = %u change player opts ",getClubID() );
		dismissEmptyRoom();
		m_jsCreateRoomOpts = prealMsg["opts"];
		if ( m_jsCreateRoomOpts["payType"].asUInt() == ePayType_AA )
		{
			m_isLackDiamond = false;
		}
		m_isClubInfoDirty = true;
	}
	break;
	case MSG_CLUB_UPDATE_PRIVILIGE:
	{
		uint8_t nRet = 0;
		uint32_t nCandinatePlayerUID = prealMsg["playerUID"].asUInt() ;
		uint32_t nPrivilige = prealMsg["privilige"].asUInt();
		do
		{
			if ( eClubPrivilige_Manager == nPrivilige && getMgrCnt() >= m_nMaxMgrCnt )
			{
				nRet = 2;
				break;
			}

			auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nTargetID);
			if (pPlayer == nullptr)
			{
				nRet = 4;
				break;
			}
			auto pmem = getMember(pPlayer->getUserUID());
			if ( pmem == nullptr || pmem->ePrivilige < eClubPrivilige_Manager || nPrivilige >= pmem->ePrivilige )
			{
				nRet = 1;
				break;
			}

			if ( false == isHaveMemeber(nCandinatePlayerUID) )
			{
				nRet = 3;
				break;
			}

			auto opt = getMember( nCandinatePlayerUID );
			if (opt->ePrivilige == nPrivilige || opt->ePrivilige >= pmem->ePrivilige )
			{
				nRet = 5;
				break;
			}
		} while ( 0 );

		Json::Value js;
		js["ret"] = nRet;
		sendMsg(js, nMsgType, nTargetID, nSenderID);
		if ( nRet )
		{
			break;
		}
		auto opt = getMember( nCandinatePlayerUID );
		opt->ePrivilige = (eClubPrivilige)nPrivilige;

		// save to db 
		saveMemberUpdateToDB(opt);

		auto p = new stClubEvent();
		p->nEventID = ++m_nMaxEventID;
		p->nEventType = eClubEvent_UpdatePrivlige;
		p->nState = eEventState_Processed;
		p->nTime = time(nullptr);
		p->jsEventDetail["uid"] = nCandinatePlayerUID;
		p->jsEventDetail["privilige"] = nPrivilige;
		p->jsEventDetail["actUID"] = nTargetID;
		addEvent(p);
	}
	break;
	case MSG_CLUB_REQ_EVENTS:
	{
		uint32_t nMaxEventID = prealMsg["clientMaxEventID"].asUInt();
		uint16_t nState = prealMsg["state"].asUInt();
		uint8_t nRet = 0;
		uint32_t nUserUID = 0;
		do 
		{
			auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nTargetID);
			if (pPlayer == nullptr)
			{
				nRet = 4;
				break;
			}
			
			nUserUID = pPlayer->getUserUID();
			auto pmem = getMember( nUserUID );
			if (pmem == nullptr || pmem->ePrivilige < eClubPrivilige_Manager)
			{
				nRet = 1;
				break;
			}

		} while ( 0 );

		if ( nRet )
		{
			Json::Value js;
			js["ret"] = nRet;
			sendMsg(js, nMsgType, nTargetID, nSenderID);
			break;
		}

		uint16_t nPageIdx = 0;
		uint8_t nMaxPage = 4; // most send pages , invoid send too many infos to clients ;

		Json::Value jsArray;
		for (auto& ref : m_vEvents)
		{
			if ( ref.second->nState != nState || ref.second->nEventID <= nMaxEventID )
			{
				continue;
			}

			Json::Value jsEvnt;
			jsEvnt["eventID"] = ref.second->nEventID;
			jsEvnt["detail"] = ref.second->jsEventDetail;
			jsEvnt["type"] = ref.second->nEventType;
			jsEvnt["state"] = ref.second->nState;
			jsEvnt["time"] = ref.second->nTime;
			jsArray[jsArray.size()] = jsEvnt;
			if ( jsArray.size() == 10 )
			{
				Json::Value jsMsg;
				jsMsg["pageIdx"] = nPageIdx;
				jsMsg["clubID"] = getClubID();
				jsMsg["vEvents"] = jsArray;
				jsMsg["ret"] = nRet;
				sendMsg(jsMsg, nMsgType, nTargetID, nSenderID);

				// go on next page ;
				jsArray.clear();
				++nPageIdx;
			}

			if ( nPageIdx >= nMaxPage )
			{
				LOGFMTD( "club id = %u can send too my events to playe UID = %u",getClubID(), nUserUID );
				break;
			}
		}

		Json::Value jsMsg;
		jsMsg["pageIdx"] = nPageIdx;
		jsMsg["clubID"] = getClubID();
		jsMsg["vEvents"] = jsArray;
		jsMsg["ret"] = nRet;
		sendMsg(jsMsg, nMsgType, nTargetID, nSenderID);
	}
	break;
	case MSG_CLUB_PROCESS_EVENT:
	{
		uint32_t nEventID = prealMsg["eventID"].asUInt();
		auto pit = m_vEvents.find(nEventID);
		uint8_t nRet = 0;
		do
		{
			if (prealMsg["detial"].isNull())
			{
				nRet = 5;
				break;
			}

			if ( m_vEvents.end() == pit )
			{
				nRet = 1;
				break;
			}

			if ( pit->second == nullptr || pit->second->nState != eEventState_WaitProcesse )
			{
				nRet = 2;
				break;
			}

			auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nTargetID);
			if (pPlayer == nullptr)
			{
				nRet = 4;
				break;
			}

			auto pmem = getMember( pPlayer->getUserUID() );
			if ( pmem == nullptr || pmem->ePrivilige < eClubPrivilige_Manager)
			{
				nRet = 3;
				break;
			}
		} while ( 0 );

		prealMsg["ret"] = nRet;
		sendMsg(prealMsg, nMsgType, nTargetID, nSenderID);
		if ( nRet ) 
		{
			break;
		}
		
		auto jsDetail = prealMsg["detial"];
		auto pEvent = pit->second;
		if ( pEvent->nEventType == eClubEvent_ApplyJoin )
		{
			bool nIsAgree = jsDetail["isAgree"].asUInt() == 1;
			pEvent->nState = eEventState_Processed;
			pEvent->nTime = time(nullptr);
			uint32_t nApplyUID = pEvent->jsEventDetail["uid"].asUInt();
			pEvent->jsEventDetail["respUID"] = nTargetID;
			pEvent->jsEventDetail["isAgree"] = jsDetail["isAgree"];
			if ( nIsAgree )
			{
				addMember( nApplyUID );
			}

			// post mail to the player ;
			Json::Value js;
			js["clubID"] = getClubID();
			js["clubName"] = m_strName;
			js["nIsAgree"] = nIsAgree;
			postMail(nApplyUID, eMail_ResponeClubApplyJoin, js, eMailState_SysProcessed);
			saveEventToDB(pEvent->nEventID, false );
		}
	}
	break;
	case MSG_CLUB_REQ_INFO:
	{
		Json::Value jsInfo;
		jsInfo["name"] = m_strName;
		jsInfo["clubID"] = m_nClubID;
		jsInfo["opts"] = m_jsCreateRoomOpts;
		jsInfo["capacity"] = m_nCapacity;
		jsInfo["maxEventID"] = m_nMaxEventID;
		jsInfo["curCnt"] = m_vMembers.size();
		jsInfo["creator"] = getCreatorUID();
		jsInfo["diamond"] = getDiamond();
		jsInfo["state"] = m_nState;
		jsInfo["inviteCnt"] = m_vInvitations.size();
		jsInfo["notice"] = m_strNotice;

		if ( isEnablePointsRestrict() )
		{
			auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nTargetID);
			if (pPlayer)
			{
				auto pmem = getMember(pPlayer->getUserUID());
				if ( pmem )
				{
					jsInfo["selfPoint"] = pmem->nOffsetPoints;
					jsInfo["selfInitPoint"] = pmem->nInitPoints;
				}
			}
		}

		Json::Value jsMgrs;
		for (auto& ref : m_vMembers)
		{
			if (ref.second->ePrivilige != eClubPrivilige_Manager)
			{
				continue;
			}
			jsMgrs[jsMgrs.size()] = ref.second->nPlayerUID;
			if (jsMgrs.size() >= m_nMaxMgrCnt)
			{
				break;
			}
		}
		jsInfo["mgrs"] = jsMgrs;
		sendMsg(jsInfo, nMsgType, nTargetID, nSenderID);
	}
	break;
	case MSG_CLUB_REQ_ROOMS:
	{
		Json::Value jsFullRooms;
		Json::Value jsEmptyRooms;
		for (auto& ref : m_vFullRooms)
		{
			//Json::Value jsItem;
			//jsItem["id"] = ref.nRoomID;
			//jsItem["idx"] = ref.nRoomIdx;
			jsFullRooms[jsFullRooms.size()] = ref.nRoomID;
		}

		for (auto& ref : m_vEmptyRooms )
		{
			//Json::Value jsItem;
			//jsItem["id"] = ref.nRoomID;
			//jsItem["idx"] = ref.nRoomIdx;
			jsEmptyRooms[jsEmptyRooms.size()] = ref.nRoomID;
		}

		Json::Value jsmsg;
		jsmsg["clubID"] = getClubID();
		jsmsg["fullRooms"] = jsFullRooms;
		jsmsg["emptyRooms"] = jsEmptyRooms;
		sendMsg(jsmsg, nMsgType, nTargetID, nSenderID);
	}
	break;
	case MSG_CLUB_REQ_PLAYERS:
	{
		uint16_t nPages = 0;
		bool isSendPoints = false;
		if ( isEnablePointsRestrict() )
		{
			auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nTargetID);
			if ( pPlayer )
			{
				auto pmem = getMember(pPlayer->getUserUID());
				isSendPoints = pmem && pmem->ePrivilige >= eClubPrivilige_Manager;
			}
		}

		Json::Value jsPlayers;
		for ( auto& ref : m_vMembers )
		{
			auto pMem = ref.second;
			Json::Value jsp;
			jsp["uid"] = pMem->nPlayerUID;
			jsp["privilige"] = pMem->ePrivilige;

			auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(pMem->nPlayerUID);
			if (pPlayer && pPlayer->isPlayerReady()) {
				jsp["name"] = pPlayer->getBaseData()->getPlayerName();
				jsp["headIcon"] = pPlayer->getBaseData()->getHeadIcon();
				jsp["isOnline"] = pPlayer->isState(CPlayer::ePlayerState_Online) == true ? 1 : 0;
			}

			if (isSendPoints)
			{
				jsp["initPoint"] = pMem->nInitPoints;
				jsp["offsetPoint"] = pMem->nOffsetPoints;
			}

			jsPlayers[jsPlayers.size()] = jsp;
			if ( 10 == jsPlayers.size() )
			{
				Json::Value js;
				js["pageIdx"] = nPages++;
				js["players"] = jsPlayers;
				sendMsg(js, nMsgType, nTargetID, nSenderID);
				jsPlayers.clear();
			}
		}

		Json::Value js;
		js["pageIdx"] = nPages++;
		js["players"] = jsPlayers;
		sendMsg(js, nMsgType, nTargetID, nSenderID);
	}
	break;
	case MSG_CLUB_INVITE_JOIN:
	{
		auto jsUIDs = prealMsg["invites"];
		uint8_t nRet = 0;
		do
		{
			if (jsUIDs.isNull() || jsUIDs.size() == 0)
			{
				nRet = 1;
				break;
			}

			auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nTargetID);
			if (pPlayer == nullptr)
			{
				nRet = 4;
				break;
			}

			auto pmem = getMember(pPlayer->getUserUID());
			if (pmem == nullptr || pmem->ePrivilige < eClubPrivilige_Manager)
			{
				nRet = 2;
				break;
			}

			for (uint32_t nIdx = 0; nIdx < jsUIDs.size(); ++nIdx)
			{
				auto uid = jsUIDs[nIdx].asUInt();
				if ( isHaveMemeber(uid) )
				{
					nRet = 3;
					break;
				}

				for (auto ref : m_vInvitations)
				{
					if (uid == ref->nUserUID)
					{
						nRet = 5;
						break;
					}
				}

				if ( nRet )
				{
					break;
				}
			}
		} while ( 0 );

		prealMsg["ret"] = nRet;
		sendMsg(prealMsg, nMsgType, nTargetID, nSenderID);
		if ( nRet )
		{
			break;
		}

		// do send invitation ;
		auto nTime = time(nullptr);
		for (uint32_t nIdx = 0; nIdx < jsUIDs.size(); ++nIdx)
		{
			auto pI = new stInvitation();
			pI->nUserUID = jsUIDs[nIdx].asUInt();
			pI->nTime = nTime;
			m_vInvitations.push_back(pI);

			// do send invation mail ; //  { clubID : 23 , clubName : "abc"  }
			Json::Value js;
			js["clubID"] = getClubID();
			js["clubName"] = m_strName;
			js["mgrID"] = nTargetID;
			postMail(pI->nUserUID, eMail_ClubInvite, js, eMailState_WaitPlayerAct );
		}
	}
	break;
	case MSG_CLUB_RESPONE_INVITE:
	{
		uint8_t nRet = 0;
		uint32_t nUserUID = 0;
		bool isAgree = prealMsg["nIsAgree"].asUInt() == 1;
		do
		{
			if ( m_vMembers.size() >= m_nCapacity )
			{
				nRet = 1;
				break;
			}

			auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nTargetID);
			if (pPlayer == nullptr)
			{
				nRet = 4;
				break;
			}
			nUserUID = pPlayer->getUserUID();
			
			nRet = 2;
			for ( auto& ref : m_vInvitations )
			{
				if ( ref->nUserUID == nUserUID )
				{
					nRet = 0 ;
					break;
				}
			}
		} while ( 0 );
		
		prealMsg["ret"] = nRet;
		sendMsg(prealMsg, nMsgType, nTargetID, nSenderID);

		if ( nRet )
		{
			break;
		}

		for (auto iter = m_vInvitations.begin(); iter != m_vInvitations.end(); ++iter)
		{
			if ((*iter)->nUserUID == nUserUID)
			{
				delete *iter;
				m_vInvitations.erase(iter);
				break;
			}
		}

		if ( isAgree )
		{
			addMember(nUserUID);
		}

		auto p = new stClubEvent();
		p->nEventID = ++m_nMaxEventID;
		p->nEventType = eClubEvent_RespInvite;
		p->nState = eEventState_Processed;
		p->nTime = time(nullptr);
		p->jsEventDetail["uid"] = nUserUID;
		p->jsEventDetail["nIsAgree"] = isAgree ? 1 : 0;
		addEvent(p);
	}
	break;
	case MSG_CLUB_SET_STATE:
	{
		uint8_t nRet = 0;
		uint32_t nUID = 0;
		do
		{
			auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nTargetID);
			if (pPlayer == nullptr || pPlayer->getSessionID() != nSenderID )
			{
				nRet = 4;
				break;
			}

			auto pMem = getMember(pPlayer->getUserUID());
			if (pMem == nullptr || pMem->ePrivilige < eClubPrivilige_Manager)
			{
				nRet = 1;
				break;
			}
		} while (0);
		prealMsg["ret"] = nRet;
		sendMsg(prealMsg, nMsgType, nTargetID, nSenderID);
		if (nRet)
		{
			break;
		}
		auto isPause = prealMsg["isPause"] == 1;
		decltype(m_nState) nst = 0;
		if ( isPause )
		{
			nst = 1;
		}

		if ( nst != m_nState)
		{
			m_nState = nst;
			m_isClubInfoDirty = true;
		}
	}
	break;
	case MSG_CLUB_REQ_INVITATIONS:
	{
		uint8_t nRet = 0;
		uint32_t nUserUID = 0;
		do
		{
			auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nTargetID);
			if (pPlayer == nullptr)
			{
				nRet = 4;
				break;
			}

			nUserUID = pPlayer->getUserUID();
			auto pmem = getMember(nUserUID);
			if (pmem == nullptr || pmem->ePrivilige < eClubPrivilige_Manager)
			{
				nRet = 1;
				break;
			}

		} while (0);

		if (nRet)
		{
			Json::Value js;
			js["ret"] = nRet;
			sendMsg(js, nMsgType, nTargetID, nSenderID);
			break;
		}

		Json::Value jsvIn;
		for (auto& ref : m_vInvitations)
		{
			jsvIn[jsvIn.size()] = ref->nUserUID;
		}
		prealMsg["ret"] = nRet;
		prealMsg["invitations"] = jsvIn;
		sendMsg(prealMsg, nMsgType, nTargetID, nSenderID);
	}
	break;
	case MSG_CLUB_UPDATE_NOTICE:
	{
		uint8_t nRet = 0;
		uint32_t nUserUID = 0;
		do
		{
			auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nTargetID);
			if (pPlayer == nullptr)
			{
				nRet = 4;
				break;
			}

			nUserUID = pPlayer->getUserUID();
			auto pmem = getMember(nUserUID);
			if (pmem == nullptr || pmem->ePrivilige < eClubPrivilige_Manager)
			{
				nRet = 1;
				break;
			}

		} while (0);

		if (nRet)
		{
			Json::Value js;
			js["ret"] = nRet;
			sendMsg(js, nMsgType, nTargetID, nSenderID);
			break;
		}

		auto str = prealMsg["notice"].asString();
		m_strNotice = str;
		m_isClubInfoDirty = true;
		prealMsg["ret"] = 0;
		sendMsg(prealMsg, nMsgType, nTargetID, nSenderID);
	}
	break;
	case MSG_CLUB_UPDATE_NAME:
	{
		uint8_t nRet = 0;
		uint32_t nUserUID = 0;
		std::string strNewName = prealMsg["name"].asString();
		do
		{
			auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nTargetID);
			if (pPlayer == nullptr)
			{
				nRet = 4;
				break;
			}

			nUserUID = pPlayer->getUserUID();
			auto pmem = getMember(nUserUID);
			if (pmem == nullptr || pmem->ePrivilige < eClubPrivilige_Manager)
			{
				nRet = 1;
				break;
			}

			if ( strNewName == m_strName )
			{
				nRet = 2;
				break;
			}

			if (m_pMgr->isNameDuplicate(strNewName))
			{
				nRet = 3;
				break;
			}

		} while (0);

		if (nRet)
		{
			Json::Value js;
			js["ret"] = nRet;
			sendMsg(js, nMsgType, nTargetID, nSenderID);
			break;
		}
		m_strName = strNewName;
		m_isClubInfoDirty = true;
		prealMsg["ret"] = 0;
		sendMsg(prealMsg, nMsgType, nTargetID, nSenderID);
	}
	break;
	case MSG_CLUB_UPDATE_DIAMOND:
	{
		prealMsg["diamond"] = getDiamond();
		sendMsg(prealMsg, nMsgType, nTargetID, nSenderID);
	}
	break;
	case MSG_CLUB_SET_PLAYER_INIT_POINTS:
	{
		uint8_t nRet = 0;
		uint32_t nTargetMemberUID = prealMsg["uid"].asUInt();
		do
		{
			auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nTargetID);
			if (pPlayer == nullptr || pPlayer->getSessionID() != nSenderID)
			{
				nRet = 4;
				break;
			}

			auto pMem = getMember(pPlayer->getUserUID());
			if (pMem == nullptr || pMem->ePrivilige < eClubPrivilige_Manager)
			{
				nRet = 1;
				break;
			}

			auto pTargetMem = getMember(nTargetMemberUID);
			if (pMem == nullptr)
			{
				nRet = 2;
				break;
			}

			if ( prealMsg["points"].isInt() == false || prealMsg["points"].asInt() < 0)
			{
				nRet = 3;
				break;
			}

			// do opeater 
			pTargetMem->nInitPoints = prealMsg["points"].asInt();

			auto p = new stClubEvent();
			p->nEventID = ++m_nMaxEventID;
			p->nEventType = eClubEvent_SetPlayerInitPoints;
			p->nState = eEventState_Processed;
			p->nTime = time(nullptr);
			p->jsEventDetail["uid"] = nTargetMemberUID;
			p->jsEventDetail["mgrUID"] = nTargetID;
			p->jsEventDetail["points"] = pTargetMem->nInitPoints;
			addEvent(p);

			saveMemberUpdateToDB(pTargetMem);

			Json::Value jsDetail;
			jsDetail["mgrUID"] = nTargetID;
			jsDetail["initPoint"] = pTargetMem->nInitPoints;
			savePointLog(nTargetMemberUID, 2, jsDetail);

		} while (0);

		prealMsg["ret"] = nRet;
		sendMsg(prealMsg, nMsgType, nTargetID, nSenderID);
	}
	break;
	case MSG_CLUB_RESET_PLAYER_POINTS:
	{
		uint8_t nRet = 0;
		uint32_t nTargetMemberUID = prealMsg["uid"].asUInt();
		do
		{
			auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nTargetID);
			if (pPlayer == nullptr || pPlayer->getSessionID() != nSenderID)
			{
				nRet = 4;
				break;
			}

			auto pMem = getMember(pPlayer->getUserUID());
			if (pMem == nullptr || pMem->ePrivilige < eClubPrivilige_Manager)
			{
				nRet = 1;
				break;
			}

			auto pTargetMem = getMember(nTargetMemberUID);
			if (pMem == nullptr)
			{
				nRet = 2;
				break;
			}

			// do opeater 
			pTargetMem->nOffsetPoints = 0;

			auto p = new stClubEvent();
			p->nEventID = ++m_nMaxEventID;
			p->nEventType = eClubEvent_ResetPlayerPoints;
			p->nState = eEventState_Processed;
			p->nTime = time(nullptr);
			p->jsEventDetail["uid"] = nTargetMemberUID;
			p->jsEventDetail["mgrUID"] = nTargetID;
			addEvent(p);

			saveMemberUpdateToDB(pTargetMem);

			Json::Value jsDetail;
			jsDetail["mgrUID"] = nTargetID;
			jsDetail["curPoint"] = pTargetMem->nInitPoints + pTargetMem->nOffsetPoints;
			savePointLog(nTargetMemberUID,1 , jsDetail );

		} while (0);

		prealMsg["ret"] = nRet;
		sendMsg(prealMsg, nMsgType, nTargetID, nSenderID);
	}
	break;
	default:
		return false ;
	}
	return true;
}

bool Club::onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)
{
	//eAsync_ClubRoomGameOvered, // { clubID : 23 , roomID : 23 }
		//eAsync_ClubGiveBackDiamond, // { clubID : 23 ,diamond : 23 }
		//eAsync_ClubRoomStart, // { clubID : 23 , roomID : 23 }
	if ( jsReqContent["clubID"].isNull() || jsReqContent["clubID"].isUInt() == false )
	{
		return false;
	}

	uint32_t nClubID = jsReqContent["clubID"].asUInt();
	if ( getClubID() != nClubID )
	{
		return false;
	}

	switch ( nRequestType )
	{
	case eAsync_HttpCmd_ApplyJoinClub:
	{
		uint8_t nRet = 0;
		uint32_t nUID = 0;
		do
		{
			if (m_vMembers.size() >= m_nCapacity)
			{
				nRet = 3;
				break;
			}

			if (jsReqContent["uid"].isUInt() == false)
			{
				nRet = 4;
				break;
			}

			nUID = jsReqContent["uid"].asUInt();

			if (nUID == 0) {
				nRet = 4;
				break;
			}

			if (isHaveMemeber(nUID))
			{
				nRet = 1;
				break;
			}

			for (auto ref : m_vEvents)
			{
				if (ref.second->nState != eEventState_WaitProcesse || eClubEvent_ApplyJoin != ref.second->nEventType)
				{
					continue;
				}

				uint32_t uid = ref.second->jsEventDetail["uid"].asUInt();
				if (nUID == uid)
				{
					nRet = 2;
					break;
				}
			}

		} while (0);

		jsResult["ret"] = nRet;

		if (nRet)
		{
			LOGFMTE("can not apply uid = %u join, ret = %u", nUID, nRet);
			break;
		}

		auto p = new stClubEvent();
		p->nEventID = ++m_nMaxEventID;
		p->nEventType = eClubEvent_ApplyJoin;
		p->nTime = time(nullptr);
		p->jsEventDetail["uid"] = nUID;

		if (m_nAutoJoin) {
			p->nState = eEventState_WaitProcesse;
		}
		else {
			p->nState = eEventState_Processed;
			p->jsEventDetail["respUID"] = 0;
			p->jsEventDetail["isAgree"] = 1;

			addMember(nUID);
			// post mail to the player ;
			Json::Value js;
			js["clubID"] = getClubID();
			js["clubName"] = m_strName;
			js["nIsAgree"] = 1;
			postMail(nUID, eMail_ResponeClubApplyJoin, js, eMailState_SysProcessed);
		}
		
		addEvent(p);
	}
	break;
	case eAsync_ClubRoomONE:
	{
		for (auto nIdx = 0; nIdx < jsReqContent["players"].size(); ++nIdx)
		{
			auto nUID = jsReqContent["players"][nIdx].asUInt();
			decreaseMemberPlayTime(nUID);
		}
	}
	break;
	case eAsync_HttpCmd_UpdateClubRoomOpts:
	{
		//TODO...
		uint8_t nRet = 0;
		do
		{
			uint32_t nUID = jsReqContent["uid"].asUInt();
			auto pMem = getMember(nUID);
			if (pMem == nullptr || pMem->ePrivilige != eClubPrivilige_Creator)
			{
				nRet = 1;
				break;
			}
		} while (0);

		jsResult["ret"] = nRet;

		if (nRet)
		{
			break;
		}
		LOGFMTD("club id = %u change player opts ", getClubID());
		dismissEmptyRoom();
		m_jsCreateRoomOpts = jsReqContent["opts"];
		if (m_jsCreateRoomOpts["payType"].asUInt() == ePayType_AA)
		{
			m_isLackDiamond = false;
		}
		m_isClubInfoDirty = true;
	}
	break;
	case eAsync_HttpCmd_UpdateClubAutoJoin:
	{
		uint8_t nRet = 0;
		uint32_t nUID = jsReqContent["uid"].asUInt();
		do
		{
			auto pMem = getMember(nUID);
			if (pMem == nullptr || pMem->ePrivilige < eClubPrivilige_Manager)
			{
				nRet = 1;
				break;
			}
		} while (0);

		jsResult["ret"] = nRet;

		if (nRet)
		{
			break;
		}
		auto bStop = jsReqContent["state"] == 0;
		decltype(m_nAutoJoin) nst = 0;
		if (bStop == false)
		{
			nst = 1;
		}

		if (nst != m_nAutoJoin)
		{
			m_nAutoJoin = nst;
			m_isClubInfoDirty = true;
			onTimeSave();
		}
	}
	break;
	case eAsync_HttpCmd_UpdateClubCPRState:
	{
		uint8_t nRet = 0;
		uint32_t nUID = jsReqContent["uid"].asUInt();
		do
		{
			auto pMem = getMember(nUID);
			if (pMem == nullptr || pMem->ePrivilige < eClubPrivilige_Manager)
			{
				nRet = 1;
				break;
			}
		} while (0);

		jsResult["ret"] = nRet;

		if (nRet)
		{
			break;
		}
		auto bStop = jsReqContent["state"] == 0;
		decltype(m_nCreatePRoomState) nst = 0;
		if (bStop == false)
		{
			nst = 1;
		}

		if (nst != m_nCreatePRoomState)
		{
			m_nCreatePRoomState = nst;
			m_isClubInfoDirty = true;
			onTimeSave();
		}
	}
	break;
	case eAsync_HttpCmd_CloseClub:
	{
		uint8_t nRet = 0;
		uint32_t nUID = jsReqContent["uid"].asUInt();
		do
		{
			auto pMem = getMember(nUID);
			if (pMem == nullptr || pMem->ePrivilige < eClubPrivilige_Manager)
			{
				nRet = 1;
				break;
			}
		} while (0);

		jsResult["ret"] = nRet;

		if (nRet)
		{
			break;
		}
		auto isPause = jsReqContent["isPause"] == 1;
		decltype(m_nState) nst = 0;
		if (isPause)
		{
			nst = 1;
		}

		if (nst != m_nState)
		{
			m_nState = nst;
			m_isClubInfoDirty = true;
			onTimeSave();
		}
	}
	break;
	case eAsync_HttpCmd_UpdateClubPlayerPT:
	{
		uint8_t nRet = 0;
		auto nMgrUID = jsReqContent["uid"].asUInt();
		auto nTargetUID = jsReqContent["targetUID"].asUInt();
		auto nPlayTime = jsReqContent["playTime"].asUInt();
		do
		{
			if (nPlayTime > 10000) {
				nRet = 3;
				break;
			}

			if (isHaveMemeber(nTargetUID) == false)
			{
				nRet = 1;
				break;
			}

			auto pMem = getMember(nMgrUID);
			if (pMem == nullptr || pMem->ePrivilige < eClubPrivilige_Manager)
			{
				nRet = 2;
				break;
			}

		} while (0);

		jsResult["ret"] = nRet;
		if (nRet == 0)
		{
			updateMemberPlayTime(nTargetUID, nPlayTime);
		}
	}
	break;
	case eAsync_HttpCmd_ClubKickPlayer:
	{
		uint8_t nRet = 0;
		uint32_t nMgrUID = jsReqContent["uid"].asUInt();
		uint32_t nKickUID = jsReqContent["kickUID"].asUInt();
		do
		{
			if (isHaveMemeber(nKickUID) == false)
			{
				nRet = 1;
				break;
			}

			/*auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nTargetID);
			if (pPlayer == nullptr)
			{
				nRet = 4;
				break;
			}*/

			//nMgrUID = pPlayer->getUserUID();
			auto pMem = getMember(nMgrUID);
			auto pKickMem = getMember(nKickUID);
			if (pMem == nullptr || pMem->ePrivilige <= pKickMem->ePrivilige)
			{
				nRet = 2;
				break;
			}

		} while (0);

		jsResult["ret"] = nRet;
		//sendMsg(prealMsg, nMsgType, nTargetID, nSenderID);
		if (nRet == 0)
		{
			deleteMember(nKickUID);

			auto p = new stClubEvent();
			p->nEventID = ++m_nMaxEventID;
			p->nEventType = eClubEvent_Kick;
			p->nState = eEventState_Processed;
			p->nTime = time(nullptr);
			p->jsEventDetail["uid"] = nKickUID;
			p->jsEventDetail["mgrUID"] = nMgrUID;
			addEvent(p);

			// infom player ;
			Json::Value js;
			js["clubID"] = getClubID();
			js["clubName"] = m_strName;
			js["mgrID"] = nMgrUID;
			postMail(nKickUID, eMail_ClubBeKick, js, eMailState_SysProcessed);
		}
	}
	break;
	case eAsync_HttpCmd_ClubTreatEvent:
	{
		uint32_t nEventID = jsReqContent["eventID"].asUInt();
		auto nUID = jsReqContent["uid"].asUInt();
		auto pit = m_vEvents.find(nEventID);
		uint8_t nRet = 0;
		do
		{
			if (jsReqContent["detial"].isNull())
			{
				nRet = 5;
				break;
			}

			if (m_vEvents.end() == pit)
			{
				nRet = 1;
				break;
			}

			if (pit->second == nullptr || pit->second->nState != eEventState_WaitProcesse)
			{
				nRet = 2;
				break;
			}

			/*auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nTargetID);
			if (pPlayer == nullptr)
			{
				nRet = 4;
				break;
			}*/
			auto pmem = getMember(nUID);
			if (pmem == nullptr || pmem->ePrivilige < eClubPrivilige_Manager)
			{
				nRet = 3;
				break;
			}
		} while (0);

		jsResult["ret"] = nRet;

		if (nRet)
		{
			break;
		}

		auto jsDetail = jsReqContent["detial"];
		auto pEvent = pit->second;
		if (pEvent->nEventType == eClubEvent_ApplyJoin)
		{
			bool nIsAgree = jsDetail["isAgree"].asUInt() == 1;
			pEvent->nState = eEventState_Processed;
			pEvent->nTime = time(nullptr);
			uint32_t nApplyUID = pEvent->jsEventDetail["uid"].asUInt();
			pEvent->jsEventDetail["respUID"] = nUID;
			pEvent->jsEventDetail["isAgree"] = jsDetail["isAgree"];
			if (nIsAgree)
			{
				addMember(nApplyUID);
			}

			// post mail to the player ;
			Json::Value js;
			js["clubID"] = getClubID();
			js["clubName"] = m_strName;
			js["nIsAgree"] = nIsAgree;
			postMail(nApplyUID, eMail_ResponeClubApplyJoin, js, eMailState_SysProcessed);
			saveEventToDB(pEvent->nEventID, false);
		}
		else if (pEvent->nEventType == eClubEvent_ApplyLeave) {
			bool nIsAgree = jsDetail["isAgree"].asUInt() == 1;
			pEvent->nState = eEventState_Processed;
			pEvent->nTime = time(nullptr);
			uint32_t nApplyUID = pEvent->jsEventDetail["uid"].asUInt();
			pEvent->jsEventDetail["respUID"] = nUID;
			pEvent->jsEventDetail["isAgree"] = jsDetail["isAgree"];
			if (nIsAgree)
			{
				deleteMember(nApplyUID);
			}

			// post mail to the player ;
			Json::Value js;
			js["clubID"] = getClubID();
			js["clubName"] = m_strName;
			js["nIsAgree"] = nIsAgree;
			postMail(nApplyUID, eMail_ResponeClubApplyLeave, js, eMailState_SysProcessed);
			saveEventToDB(pEvent->nEventID, false);
		}
	}
	break;
	case eAsync_ClubRoomGameOvered:
	{
		uint32_t nRoomID = jsReqContent["roomID"].asUInt();
		LOGFMTD("clubID = %u room end roomID = %u", getClubID(), nRoomID );

		// check game result 
		if ( isEnablePointsRestrict() )
		{
			auto jsDetail = jsReqContent["result"];
			for (uint32_t nIdx = 0; nIdx < jsDetail.size(); ++nIdx)
			{
				auto jsp = jsDetail[nIdx];
				uint32_t nUID = jsp["uid"].asUInt();
				int32_t nOffset = jsp["offset"].asInt();
				auto pmem = getMember(nUID);
				if (pmem == nullptr)
				{
					LOGFMTE("recived player result but member is null clubID = %u , uid = %u , offset = %d", getClubID(), nUID, nOffset);
					continue;
				}
				pmem->nOffsetPoints += nOffset;

				// save member update ;
				saveMemberUpdateToDB(pmem);

				// save point log ;
				Json::Value jslogdetail;
				jslogdetail["roomOffset"] = nOffset;
				jslogdetail["roomID"] = nRoomID;
				jslogdetail["curPoint"] = pmem->nInitPoints + pmem->nOffsetPoints;
				savePointLog(nUID, 0, jslogdetail);
			}
		}

		Json::Value jsRTI;
		jsRTI["type"] = eClubRTI_RoomDismiss;
		jsRTI["detail"] = jsReqContent;
		sendIRT(jsRTI);

		// remove from full room ;
		auto iter = std::find_if(m_vFullRooms.begin(),m_vFullRooms.end(), [nRoomID](stClubRoomInfo& ref) { return ref.nRoomID == nRoomID; });
		if (iter != m_vFullRooms.end())
		{
			m_vFullRooms.erase(iter);
			break;
		}
	
		// maybe remove from empty room 
		iter = std::find_if(m_vEmptyRooms.begin(), m_vEmptyRooms.end(), [nRoomID](stClubRoomInfo& ref) { return ref.nRoomID == nRoomID; });
		if (iter != m_vEmptyRooms.end())
		{
			m_vEmptyRooms.erase(iter);
			break;
		}

		LOGFMTE( "clubID = %u , room id = %u overed , but not in club ? " , getClubID(),nRoomID );
	}
	break;
	case eAsync_ClubGiveBackDiamond:
	{
		uint32_t nBackDiamond = jsReqContent["diamond"].asUInt();
		uint32_t nRoomID = jsReqContent["roomID"].asUInt();

		// remove from full room ;
		auto iter = std::find_if(m_vFullRooms.begin(), m_vFullRooms.end(), [nRoomID](stClubRoomInfo& ref) { return ref.nRoomID == nRoomID; });
		if (iter == m_vFullRooms.end())
		{
			// maybe remove from empty room 
			iter = std::find_if(m_vEmptyRooms.begin(), m_vEmptyRooms.end(), [nRoomID](stClubRoomInfo& ref) { return ref.nRoomID == nRoomID; });
			if (iter == m_vEmptyRooms.end())
			{
				LOGFMTE("clubID = %u , room id = %u overed , but not in club can not give back diamond ? ", getClubID(), nRoomID);
				break;
			}
		}

		updateDiamond( nBackDiamond, nRoomID);
		LOGFMTD( "clubID = %u give back diamond = %u", getClubID(), nBackDiamond );

		if ( nBackDiamond > 0 )
		{
			auto jsd = jsReqContent;
			CPlayer::saveDiamondRecorder(getClubID(), eLogDiamond_ClubGiveBack, nBackDiamond, getDiamond(), jsd);
		}
	}
	break;
	case eAsync_ClubRoomStart:
	{
		uint32_t nRoomID = jsReqContent["roomID"].asUInt();
		auto iter = std::find_if(m_vEmptyRooms.begin(), m_vEmptyRooms.end(), [nRoomID](stClubRoomInfo& ref) { return ref.nRoomID == nRoomID; } );
		if (iter == m_vEmptyRooms.end())
		{
			LOGFMTE( "clubID = %u ,room id = %u start but not in empty room list",getClubID(),nRoomID );
			break;
		}
		m_vFullRooms.push_back(*iter);
		m_vEmptyRooms.erase(iter);

		Json::Value jsRTI;
		jsRTI["type"] = eClubRTI_RoomStarted;
		jsRTI["detail"] = jsReqContent;
		sendIRT(jsRTI);
	}
	break;
	case eAsync_ClubRoomSitDown:
	{
		//TODO...
		uint32_t nRoomID = jsReqContent["roomID"].asUInt();
		auto iter = std::find_if(m_vEmptyRooms.begin(), m_vEmptyRooms.end(), [nRoomID](stClubRoomInfo& ref) { return ref.nRoomID == nRoomID; });
		if (iter == m_vEmptyRooms.end())
		{
			LOGFMTE("clubID = %u ,room id = %u player sit down but not in empty room list", getClubID(), nRoomID);
			break;
		}
		iter->doPlayerSitDown();

		Json::Value jsRTI;
		jsRTI["type"] = eClubRTI_PlayerSitDown;
		jsRTI["detail"] = jsReqContent;
		sendIRT(jsRTI);
	}
	break;
	case eAsync_ClubRoomStandUp:
	{
		//TODO...
		uint32_t nRoomID = jsReqContent["roomID"].asUInt();
		auto iter = std::find_if(m_vEmptyRooms.begin(), m_vEmptyRooms.end(), [nRoomID](stClubRoomInfo& ref) { return ref.nRoomID == nRoomID; });
		if (iter == m_vEmptyRooms.end())
		{
			LOGFMTE("clubID = %u ,room id = %u player sit down but not in empty room list", getClubID(), nRoomID);
			break;
		}
		iter->doPlayerStandUp();

		Json::Value jsRTI;
		jsRTI["type"] = eClubRTI_PlayerStandUp;
		jsRTI["detail"] = jsReqContent;
		sendIRT(jsRTI);
	}
	break;
	case eAsync_ClubRoomNetStateRefreshed:
	{
		Json::Value jsRTI;
		jsRTI["type"] = eClubRTI_PlayerNetStateRefresh;
		jsRTI["detail"] = jsReqContent;
		sendIRT(jsRTI);
	}
	break;
	case eAsync_ClubCheckMemberLevel:
	{
		uint32_t nUID = jsReqContent["uid"].asUInt();
		jsResult["ret"] = 0;
		auto p = getMember(nUID);
		if (nullptr == p)
		{
			jsResult["ret"] = 1;
			break;
		}

		jsResult["level"] = p->ePrivilige;
	}
	break;
	case eAsync_ClubCheckMember:
	{
		uint32_t nUID = jsReqContent["uid"].asUInt();
		jsResult["ret"] = 0;
		auto p = getMember( nUID );
		if (nullptr == p)
		{
			jsResult["ret"] = 1;
			break;
		}
		
		if ( p->ePrivilige < eClubPrivilige_Normal )
		{
			jsResult["ret"] = 2;
			break;
		}

		if ( isEnablePointsRestrict() && p->getCurPoint() <= 0 )
		{
			jsResult["ret"] = 3;
			break;
		}
	}
	break;
	default:
		return false;
	}
	return true;
}

void Club::onTimeSave()
{
	if ( m_isClubInfoDirty )
	{
		m_isClubInfoDirty = false;
	
		Json::StyledWriter jsw;
		auto strOpts = jsw.write(m_jsCreateRoomOpts);
		Json::Value jssql;
		char pBuffer[2024] = { 0 };
		sprintf_s(pBuffer, sizeof(pBuffer), "update clubs set ownerUID = '%u', opts = '%s',name = '%s',state = '%u',cprState = '%u', autoJoin = '%u', notice = '%s' where clubID = %u limit 1 ;", getCreatorUID(), strOpts.c_str(), m_strName.c_str(), m_nState, m_nCreatePRoomState, m_nAutoJoin, m_strNotice.c_str(),getClubID());
		jssql["sql"] = pBuffer;
		m_pMgr->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB, rand() % 100, eAsync_DB_Update, jssql);
	}
}

void Club::addMember( uint32_t nUserUID, eClubPrivilige ePrivilige )
{
	if ( isHaveMemeber(nUserUID) )
	{
		LOGFMTE( "already have user uid = %u",nUserUID );
		return;
	}

	auto p = new stMember();
	p->ePrivilige = ePrivilige;
	p->nPlayerUID = nUserUID;
	m_vMembers[p->nPlayerUID] = p;

	auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nUserUID);
	if (pPlayer)
	{
		pPlayer->getBaseData()->onJoinClub(getClubID());

		if (p->ePrivilige == eClubPrivilige_Creator) {
			pPlayer->getBaseData()->onCreatedClub(getClubID());
		}
	}
	else
	{
		Json::Value js;
		js["clubID"] = getClubID();
		postMail(nUserUID, eMail_ClubJoin, js, eMailState_WaitSysAct );
	}

	// save to db 
	Json::StyledWriter jsw;
	Json::Value jssql;
	char pBuffer[512] = { 0 };
	sprintf_s(pBuffer,sizeof(pBuffer) ,"insert into clubmember ( clubID,uid,privilige ) values ( %u,%u,%u );", getClubID(), nUserUID, ePrivilige );
	jssql["sql"] = pBuffer;
	m_pMgr->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB,rand() % 100, eAsync_DB_Add, jssql);
}

void Club::deleteMember(uint32_t nUserUID)
{
	auto p = m_vMembers.find( nUserUID );
	if ( p != m_vMembers.end() )
	{
		delete p->second;
		p->second = nullptr;
		m_vMembers.erase( p );

		// remove from player
		auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nUserUID);
		if ( pPlayer )
		{
			pPlayer->getBaseData()->onLeaveClub(getClubID());
		}
		else
		{
			Json::Value js;
			js["clubID"] = getClubID();
			postMail(nUserUID, eMail_ClubLeave, js, eMailState_WaitSysAct);
		}

		// save to db 
		Json::StyledWriter jsw;
		Json::Value jssql;
		char pBuffer[512] = { 0 };
		sprintf_s(pBuffer, sizeof(pBuffer), "delete from clubmember where clubID = %u and uid = %u limit 1 ;", getClubID(), nUserUID);
		jssql["sql"] = pBuffer;
		m_pMgr->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB,rand() % 100, eAsync_DB_Delete, jssql);
	}
}

bool Club::isHaveMemeber(uint32_t nUserUID)
{
	auto p = m_vMembers.find(nUserUID);
	return p != m_vMembers.end();
}

Club::stMember* Club::getMember( uint32_t nUserUID )
{
	auto p = m_vMembers.find(nUserUID);
	if ( p == m_vMembers.end() )
	{
		return nullptr;
	}
	return p->second;
}

bool Club::sendMsg( Json::Value& recvValue, uint16_t nMsgID, uint32_t nSenderUID, uint32_t nTargetID, uint8_t nTargetPort )
{
	return m_pMgr->sendMsg(recvValue, nMsgID, nSenderUID,nTargetID,nTargetPort);
}

uint32_t Club::getClubID()
{
	return m_nClubID;
}

uint16_t Club::getMgrCnt()
{
	uint16_t nMgrCnt = 0;
	for (auto ref : m_vMembers)
	{
		if (ref.second && ref.second->ePrivilige == eClubPrivilige_Manager)
		{
			++nMgrCnt;
		}
	}
	return nMgrCnt;
}

uint32_t Club::getCreatorUID()
{
	for (auto ref : m_vMembers)
	{
		if (ref.second && ref.second->ePrivilige == eClubPrivilige_Creator )
		{
			return ref.second->nPlayerUID;
		}
	}
	return 0;
}

uint32_t Club::getCreatorDiamond() {
	auto nCreatorUID = getCreatorUID();
	if (nCreatorUID) {
		auto pCreator = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nCreatorUID);
		if (pCreator) {
			return pCreator->getBaseData()->getDiamoned();
		}
	}
	return 0;
}

bool Club::isCreatorReady() {
	auto nCreatorUID = getCreatorUID();
	if (nCreatorUID) {
		auto pCreator = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nCreatorUID);
		if (pCreator) {
			return pCreator->isPlayerReady();
		}
	}
	return false;
}

bool Club::modifyCreatorDiamond(int32_t nDiamond) {
	auto nCreatorUID = getCreatorUID();
	if (nCreatorUID) {
		auto pCreator = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nCreatorUID);
		if (pCreator) {
			return pCreator->getBaseData()->modifyMoney(nDiamond, true);
		}
	}

	return false;
}

void Club::onWillDismiss()
{
	for (auto& ref : m_vMembers)
	{
		// infom player ;
		Json::Value js;
		js["clubID"] = getClubID();
		js["clubName"] = m_strName;
		postMail(ref.second->nPlayerUID, eMail_ClubDismiss, js, eMailState_SysProcessed );
	}

	do
	{
		auto iter = m_vMembers.begin();
		if (iter == m_vMembers.end())
		{
			break;
		}

		if ( iter->second == nullptr )
		{
			m_vMembers.erase(iter);
			continue;
		}
		LOGFMTD( "dismiss clubid = %u remove uid = %u", getClubID(), iter->second->nPlayerUID );
		deleteMember( iter->second->nPlayerUID );
	} while ( 1 );

	// dissmiss room
	dismissEmptyRoom( true );
}

void Club::update(float fDeta)
{
	// check need open room ;
	m_fDelayTryCreateRoom -= fDeta;
	if ( m_fDelayTryCreateRoom < 0.1 )
	{
		m_fDelayTryCreateRoom = 0;
	}
	updateCreateRoom();
}

uint8_t Club::getEmptyAutoCreatRoomCnt() {
	uint8_t nCnt = 0;
	for (auto& ref : m_vEmptyRooms) {
		if (ref.bPrivate) {
			continue;
		}
		nCnt++;
	}
	return nCnt;
}

void Club::clearLackDiamond() {
	auto nRoomType = m_jsCreateRoomOpts["gameType"].asUInt();
	if (eGame_FXMJ == nRoomType) {
		m_isLackDiamond = false;
	}
}

void Club::updateCreateRoom()
{
	if ( m_isFinishReadEvent && m_isFinishReadMembers && false == m_isLackDiamond && false == m_isCreatingRoom && false == isPasuseState() && getEmptyAutoCreatRoomCnt() < MAX_EMPTY_ROOM_CNT && m_fDelayTryCreateRoom < 0.1 )
	{
		m_isCreatingRoom = true;
		auto nRoomType = m_jsCreateRoomOpts["gameType"].asUInt();
		m_jsCreateRoomOpts["uid"] = 0;
		m_jsCreateRoomOpts["clubID"] = getClubID();
		//m_jsCreateRoomOpts["diamond"] = getDiamond();
		m_jsCreateRoomOpts["clubName"] = getName();
		auto nPort = getTargetPortByGameType(nRoomType);
		if ( nPort >= ID_MSG_PORT_MAX)
		{
			m_isCreatingRoom = false;
			LOGFMTE( "targe port error can not create room game type = %u",nRoomType );
			m_nState = 1;
			return;
		}

		if (ID_MSG_PORT_FXMJ == nPort) {
			if (isCreatorReady() == false) {
				m_isCreatingRoom = false;
				m_fDelayTryCreateRoom = 10;
				LOGFMTE("club %u creator is not ready, delay try to create empty room", getClubID());
				return;
			}
			m_jsCreateRoomOpts["diamond"] = getCreatorDiamond();
		}
		else {
			m_jsCreateRoomOpts["diamond"] = getDiamond();
		}

		auto asyq = m_pMgr->getSvrApp()->getAsynReqQueue();
		m_jsCreateRoomOpts["roomIdx"] = ++m_nMaxRoomIdx;
		if ( m_nMaxRoomIdx >= 99 )
		{
			m_nMaxRoomIdx = 0;
		}

		uint8_t nSeatCnt = m_jsCreateRoomOpts["seatCnt"].asUInt();

		asyq->pushAsyncRequest( nPort , 0, eAsync_ClubCreateRoom, m_jsCreateRoomOpts, [this, nPort, nSeatCnt](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut) {
			m_isCreatingRoom = false;
			if ( isTimeOut )
			{
				return;
			}

			uint8_t nRet = retContent["ret"].asUInt();
			if ( 0 == nRet )
			{
				uint32_t nRoomID = retContent["roomID"].asUInt();
				if (checkDiamondEnough(retContent["diamondFee"].asUInt(), nRoomID)) {
					onCreateEmptyRoom(retContent["roomID"].asUInt(), retContent["diamondFee"].asInt(), retContent["roomIdx"].asInt(), nSeatCnt);
				}
				else {
					Json::Value js;
					js["roomID"] = nRoomID;
					m_pMgr->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(nPort, 0, eAsync_ClubDismissRoom, js, nRoomID);
				}

				return;
			}

			// ret:  1 ,diamond is not enough ,  2  admin stoped create room , 3  room ptr is null, 4 room id run out.
			LOGFMTE( "club id = %u create room ret = %u",getClubID(),nRet );
			if ( 1 == nRet )
			{
				m_isLackDiamond = true;
				return;
			}

			m_fDelayTryCreateRoom = 60 * 8; // delay create room ; error so , try later ;
		});
	}
}

uint16_t Club::getTargetPortByGameType(uint32_t nGameType)
{
	switch (nGameType)
	{
	case eGame_NiuNiu:
	{
		return ID_MSG_PORT_NIU_NIU;
	}
	break;
	case eGame_BiJi:
	{
		return ID_MSG_PORT_BI_JI;
	}
	break;
	case eGame_CYDouDiZhu:
	case eGame_JJDouDiZhu:
	{
		return ID_MSG_PORT_DOU_DI_ZHU;
	}
	break;
	case eGame_Golden:
	{
		return ID_MSG_PORT_GOLDEN;
	}
	break;
	case eGame_MQMJ:
	{
		return ID_MSG_PORT_MQMJ;
	}
	break;
	case eGame_LuoMJ:
	{
		return ID_MSG_PORT_LUOMJ;
	}
	break;
	case eGame_FXMJ:
	{
		return ID_MSG_PORT_FXMJ;
	}
	break;
	case eGame_CFMJ:
	{
		return ID_MSG_PORT_CFMJ;
	}
	break;
	case eGame_AHMJ:
	{
		return ID_MSG_PORT_AHMJ;
	}
	break;
	case eGame_NCMJ:
	{
		return ID_MSG_PORT_NCMJ;
	}
	break;
	case eGame_DDMJ:
	{
		return ID_MSG_PORT_DDMJ;
	}
	break;
	default:
		break;
	}
	return ID_MSG_PORT_MAX;
}

void Club::onCreateEmptyRoom(uint32_t nRoomID, int32_t nDiamondFee, uint32_t nRoomIdx, uint8_t nSeatCnt, bool bPrivate)
{
	stClubRoomInfo ci;
	ci.nRoomID = nRoomID;
	ci.nRoomIdx = nRoomIdx;
	ci.bPrivate = bPrivate;
	ci.nMaxPlayerCnt = nSeatCnt;
	ci.nCurPlayerCnt = 0;
	m_vEmptyRooms.push_back(ci);
	updateDiamond( nDiamondFee * -1 , nRoomID);

	if ( abs( nDiamondFee ) > 0 )
	{
		Json::Value jsd;
		jsd["roomID"] = nRoomID;
		CPlayer::saveDiamondRecorder(getClubID(), eLogDiamond_ClubConsume, nDiamondFee * -1, getDiamond(), jsd);
	}

	m_isCreatingRoom = false;

	Json::Value jsRTI, jsDetail;
	jsRTI["type"] = eClubRTI_RoomCreated;
	jsDetail["roomID"] = nRoomID;
	jsDetail["clubID"] = getClubID();
	jsRTI["detail"] = jsDetail;
	sendIRT(jsRTI);

	LOGFMTD("created room id = %u consume diamond = %d, clubid = %u, final diamond = %u",nRoomID,nDiamondFee,getClubID(),getDiamond() );
}

bool Club::checkDiamondEnough(uint32_t nOffset, uint32_t nRoomID)
{
	auto nPort = ClubManager::parePortTypte(nRoomID);
	if (nPort == ID_MSG_PORT_FXMJ) {
		auto nCreatorUID = getCreatorUID();
		if (nCreatorUID) {
			auto pCreator = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nCreatorUID);
			if (pCreator && pCreator->isPlayerReady()) {
				return pCreator->getBaseData()->getDiamoned() >= nOffset;
			}
		}
	}
	else {
		return m_nDiamond >= nOffset;
	}
	return false;
}

void Club::updateDiamond(int32_t nDiamond, uint32_t nRoomID)
{
	auto nPort = ClubManager::parePortTypte(nRoomID);
	if (nPort == ID_MSG_PORT_FXMJ) {
		modifyCreatorDiamond(nDiamond);
		if (nDiamond > 0)
		{
			m_isLackDiamond = false;
		}
		return;
	}

	if (nDiamond < (int32_t)0 && abs(nDiamond) > (int32_t)getDiamond())
	{
		LOGFMTE( "diamond = %u is too few , need cnt = %u",getDiamond(), nDiamond );
		m_nDiamond = 0;
		return;
	}
	m_nDiamond += nDiamond;

	if ( nDiamond > 0 )
	{
		m_isLackDiamond = false;
	}

	Json::Value jssql;
	char pBuffer[512] = { 0 };
	sprintf_s(pBuffer, sizeof(pBuffer), "update clubs set diamond = %u where clubID = %u limit 1 ;", m_nDiamond, getClubID());
	jssql["sql"] = pBuffer;
	m_pMgr->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB, rand() % 100, eAsync_DB_Update, jssql);
}

bool Club::canDismiss()
{
	return m_vFullRooms.empty();
}

bool Club::setIsEnablePointRestrict(bool isEnable)
{
	if ( isEnable == isEnablePointsRestrict())
	{
		return false;
	}

	m_jsCreateRoomOpts["pointRct"] = isEnable ? 1 : 0;
	m_isClubInfoDirty = true;
	return true;
}

void Club::readClubDetail()
{
	// read max event id  ;
	Json::Value jssql;
	char pBuffer[512] = { 0 };
	sprintf_s(pBuffer, sizeof(pBuffer), "select max(eventID) as 'maxEventID' from clubevent where clubID = %u",  getClubID() );
	jssql["sql"] = pBuffer;
	m_pMgr->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB,rand() % 100, eAsync_DB_Select, jssql, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut ) {
		uint8_t nRow = retContent["afctRow"].asUInt();
		Json::Value jsData = retContent["data"];
		if (nRow == 0)
		{
			LOGFMTD("do not read maxEventID max serial number");
		}
		else
		{
			Json::Value jsRow = jsData[(uint32_t)0];
			m_nMaxEventID = jsRow["maxEventID"].asUInt();
			LOGFMTD("read max maxEventID = %u , clubid = %u", m_nMaxEventID , getClubID() );
		}
	});

	// read member ;
	readClubMemebers(0);
	// read event ;
	readClubEvents(0);
}

void Club::readClubEvents( uint32_t nAlreadyCnt )
{
	uint32_t nRreadTimeStamp = (uint32_t)time( nullptr ) - 60*60*24*2; // two day ago;
	auto asyq = m_pMgr->getSvrApp()->getAsynReqQueue();
	std::ostringstream ss;
	ss << "SELECT eventID,eventType,jsDetail,nTime,nState FROM clubevent where clubID = " << getClubID() << " and nTime > " << nRreadTimeStamp << " limit 20 OFFSET " << nAlreadyCnt << ";";
	Json::Value jsReq;
	jsReq["sql"] = ss.str();
	asyq->pushAsyncRequest(ID_MSG_PORT_DB,rand() % 100, eAsync_DB_Select, jsReq, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut ) {
		uint32_t nAft = retContent["afctRow"].asUInt();
		auto jsData = retContent["data"];
		if (nAft == 0 || jsData.isNull())
		{
			LOGFMTD("finish club events 2 id = %u ", getClubID());
			m_isFinishReadEvent = true;
			return;
		}

		for (uint32_t nRowIdx = 0; nRowIdx < nAft; ++nRowIdx)
		{
			auto jsRow = jsData[nRowIdx];

			auto nEventID = jsRow["eventID"].asUInt();
			auto iterEvent = m_vEvents.find( nEventID );
			if ( iterEvent != m_vEvents.end() )
			{
				LOGFMTE("why have duplicate event club id = %u, stop read eventID = %u ", getClubID(), nEventID );
				m_isFinishReadEvent = true;
				return;
			}

			// must not invoker addMember() ;
			auto p = new stClubEvent();
			p->nEventID = nEventID;
			p->nEventType = (eClubEvent)jsRow["eventType"].asUInt();
			p->nTime = jsRow["nTime"].asUInt();
			p->nState = jsRow["nState"].asUInt();

			Json::Reader jsr;
			jsr.parse(jsRow["jsDetail"].asString(), p->jsEventDetail);

			m_vEvents[nEventID] = p;
		}

		auto nNewOffset = m_vEvents.size();
		if (nNewOffset % 20 != 0 || nAft < 20)  // finish read clubs ;
		{
			LOGFMTD("finish club events  id = %u ", getClubID());
			m_isFinishReadEvent = true;
			return;
		}

		if ( nNewOffset > 600 )
		{
			LOGFMTD(" two many cnts finish club events  id = %u ", getClubID());
			m_isFinishReadEvent = true;
			return;
		}

		// not finish , go on read 
		readClubEvents(nNewOffset);
	});
}

void Club::readClubMemebers( uint32_t nAlreadyReadCnt)
{
	auto asyq = m_pMgr->getSvrApp()->getAsynReqQueue();
	std::ostringstream ss;
	ss << "SELECT uid,privilige,offsetPoints,initPoints,playTime FROM clubmember where clubID = " << getClubID() << " limit 20 OFFSET " << nAlreadyReadCnt << ";";
	Json::Value jsReq;
	jsReq["sql"] = ss.str();
	asyq->pushAsyncRequest(ID_MSG_PORT_DB,rand() % 100, eAsync_DB_Select, jsReq, [this, asyq](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut ) {
		uint32_t nAft = retContent["afctRow"].asUInt();
		auto jsData = retContent["data"];
		if (nAft == 0 || jsData.isNull())
		{
			LOGFMTD("finish club members 2 id = %u ", getClubID());
			m_isFinishReadMembers = true;
			return;
		}

		for (uint32_t nRowIdx = 0; nRowIdx < nAft; ++nRowIdx)
		{
			auto jsRow = jsData[nRowIdx];

			auto nUID = jsRow["uid"].asUInt();
			if ( isHaveMemeber( nUID ) )
			{
				LOGFMTE("why have duplicate clubid = %u, stop read uid = %u ",getClubID() ,nUID );
				m_isFinishReadMembers = true;
				return;
			}

			// must not invoker addMember() ;
			auto p = new stMember();
			p->nPlayerUID = nUID;
			p->ePrivilige = (eClubPrivilige)jsRow["privilige"].asUInt();
			p->nInitPoints = jsRow["initPoints"].asInt();
			p->nOffsetPoints = jsRow["offsetPoints"].asInt();
			p->nPlayTime = jsRow["playTime"].asUInt();
			m_vMembers[nUID] = p;

			if (p->ePrivilige == eClubPrivilige_Creator) {
				Json::Value jsLogin;
				jsLogin["uid"] = nUID;
				jsLogin["sessionID"] = 0;
				jsLogin["ip"] = "0";
				jsLogin["clubID"] = getClubID();
				asyq->pushAsyncRequest(ID_MSG_PORT_DATA, nUID, eAsync_Player_Logined, jsLogin);
			}
		}

		auto nNewOffset = m_vMembers.size();
		if ( nNewOffset % 20 != 0 || nAft < 20 )  // finish read clubs ;
		{
			LOGFMTD("finish club members  id = %u " , getClubID() );
			m_isFinishReadMembers = true;
			return;
		}

		// not finish , go on read 
		readClubMemebers(nNewOffset);
	});
}

void Club::saveEventToDB(uint32_t nEventID, bool isAdd)
{
	auto pEventIter = m_vEvents.find(nEventID);
	if ( pEventIter == m_vEvents.end())
	{
		LOGFMTE( "just event add , can not find ?  event id = %u club id = %u",nEventID, getClubID() );
		return;
	}

	auto pEvent = pEventIter->second;
	Json::StyledWriter jsw;
	auto strDetail = jsw.write(pEvent->jsEventDetail);
	// save to db 
	Json::Value jssql;
	char pBuffer[1024] = { 0 };
	if ( isAdd ) 
	{
		sprintf_s(pBuffer, sizeof(pBuffer), "insert into clubevent ( clubID,eventID,eventType,jsDetail,nTime,nState ) values ( %u,%u,%u,'%s',%u,%u );", getClubID(), pEvent->nEventID,pEvent->nEventType, strDetail.c_str(),(uint32_t)pEvent->nTime,pEvent->nState );
	}
	else  // update 
	{
		sprintf_s(pBuffer, sizeof(pBuffer), "update clubevent set jsDetail = '%s', nState = %u , nTime = %u where clubID = %u and eventID = %u limit 1", strDetail.c_str(),pEvent->nState, (uint32_t)pEvent->nTime,getClubID(), pEvent->nEventID);
	}
	
	jssql["sql"] = pBuffer;
	m_pMgr->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB,rand() % 100 , isAdd ? eAsync_DB_Add : eAsync_DB_Update , jssql);
}

bool Club::addEvent( stClubEvent* pEvent )
{
	auto iter = m_vEvents.find( pEvent->nEventID );
	if (iter != m_vEvents.end())
	{
		LOGFMTE( "alrady have event id = %u , clubID = %u",pEvent->nEventID,getClubID() );
		return false;
	}
	m_vEvents[pEvent->nEventID] = pEvent;

	saveEventToDB(pEvent->nEventID,true);

	if ( m_vEvents.size() > 200 )
	{
		auto pTeamieralTime = time(nullptr) - 60*60*24*2; // two day ;
		std::vector<uint32_t> vWillDel;
		for (auto& ref : m_vEvents)
		{
			if (ref.second->nTime < pTeamieralTime)
			{
				vWillDel.push_back( ref.second->nEventID );
			}
		}

		for (auto& ref : vWillDel)
		{
			auto iter = m_vEvents.find(ref);
			delete iter->second;
			iter->second = nullptr;
			m_vEvents.erase( iter );
		}
	}
	return true;
}

void Club::postMail( uint32_t nTargetID, eMailType eType, Json::Value& jsContent, eMailState eState )
{
	auto pMailModule = ((DataServerApp*)m_pMgr->getSvrApp())->getMailModule();
	pMailModule->postMail(nTargetID, eType, jsContent, eState );
}

void Club::dismissEmptyRoom( bool isWillDelteClub )
{
	auto nRoomType = m_jsCreateRoomOpts["gameType"].asUInt();
	for (auto& ref : m_vEmptyRooms)
	{
		Json::Value js;
		js["roomID"] = ref.nRoomID;
		uint32_t nRid = ref.nRoomID;
		if ( isWillDelteClub )
		{
			m_pMgr->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(getTargetPortByGameType(nRoomType), 0, eAsync_ClubDismissRoom, js, ref.nRoomID);
		}
		else
		{
			m_pMgr->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(getTargetPortByGameType(nRoomType), 0, eAsync_ClubDismissRoom, js, [this, nRid](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
			{
				auto iter = std::find_if(m_vEmptyRooms.begin(), m_vEmptyRooms.end(), [nRid](const stClubRoomInfo& rf) { return rf.nRoomID == nRid; });
				if (iter != m_vEmptyRooms.end())
				{
					m_vEmptyRooms.erase(iter);  // in callback function , iter will not invalid 
				}
			}, ref.nRoomID);
		}
	}
}

bool Club::isEnablePointsRestrict()
{
	return m_jsCreateRoomOpts["pointRct"].isInt() && m_jsCreateRoomOpts["pointRct"].asInt() == 1;
}

void Club::savePointLog(uint32_t nPlayerUID, uint32_t nLogType, Json::Value& jsDetail)
{
	if ( isEnablePointsRestrict() == false )
	{
		return;
	}

	Json::StyledWriter jsw;
	auto strDetail = jsw.write(jsDetail);
	// save to db 
	Json::Value jssql;
	char pBuffer[2024] = { 0 };
	sprintf_s(pBuffer, sizeof(pBuffer), "insert into clubpointrecorder ( clubID,playerUID,type,detail ) values ( %u,%u,%u,'%s');", getClubID(), nPlayerUID, nLogType, strDetail.c_str() );
	jssql["sql"] = pBuffer;
	m_pMgr->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_RECORDER_DB, rand() % 100, eAsync_DB_Add, jssql);
}

void Club::saveMemberUpdateToDB(stMember * pMem)
{
	Json::StyledWriter jsw;
	Json::Value jssql;
	char pBuffer[512] = { 0 };
	sprintf_s(pBuffer, sizeof(pBuffer), "update clubmember set privilige = %u,offsetPoints = %d,initPoints = %d, playTime = %u where clubID = %u and uid = %u limit 1 ;", pMem->ePrivilige, pMem->nOffsetPoints,pMem->nInitPoints, pMem->nPlayTime, getClubID(), pMem->nPlayerUID );
	jssql["sql"] = pBuffer;
	m_pMgr->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB, rand() % 100, eAsync_DB_Update, jssql);
}

void Club::decreaseMemberPlayTime(uint32_t nMemberUID)
{
	auto pMember = getMember(nMemberUID);
	if (pMember && pMember->nPlayTime > 1) {
		pMember->decreasePlayTime();
		saveMemberUpdateToDB(pMember);
	}
}

void Club::updateMemberPlayTime(uint32_t nMemberUID, uint32_t nPlayTime) {
	auto pMember = getMember(nMemberUID);
	if (pMember && pMember->nPlayTime > 1) {
		pMember->updatePlayTime(nPlayTime);
		saveMemberUpdateToDB(pMember);
	}
}

uint8_t Club::transferCreator(uint32_t nMemberUID) {
	//TODO...
	if (isHaveMemeber(nMemberUID) == false) {
		return 10;
	}

	auto nCreatorUID = getCreatorUID();
	if (nCreatorUID == 0) {
		return 12;
	}

	if (nCreatorUID == nMemberUID) {
		return 11;
	}

	auto pMember = getMember(nMemberUID);
	pMember->ePrivilige = eClubPrivilige_Creator;
	saveMemberUpdateToDB(pMember);

	auto pCreator = getMember(nCreatorUID);
	pCreator->ePrivilige = eClubPrivilige_Normal;
	saveMemberUpdateToDB(pCreator);

	m_isClubInfoDirty = true;

	return 0;
}

bool Club::isInIRT(uint32_t nUserID) {
	auto iter = std::find(m_vRealTimeInformation.begin(), m_vRealTimeInformation.end(), nUserID);
	return iter != m_vRealTimeInformation.end();
}

void Club::joinIRT(uint32_t nUserID) {
	if (std::find(m_vRealTimeInformation.begin(), m_vRealTimeInformation.end(), nUserID) == m_vRealTimeInformation.end()) {
		m_vRealTimeInformation.push_back(nUserID);
	}
}

void Club::removeFromIRT(uint32_t nUserID) {
	auto iter = std::find(m_vRealTimeInformation.begin(), m_vRealTimeInformation.end(), nUserID);
	while (iter != m_vRealTimeInformation.end()) {
		m_vRealTimeInformation.erase(iter);
		iter = std::find(m_vRealTimeInformation.begin(), m_vRealTimeInformation.end(), nUserID);
	}
}

void Club::sendIRT(Json::Value& jsMsg) {
	std::vector<uint32_t> vInvalid;
	for (auto nUserID : m_vRealTimeInformation) {
		auto pPlayer = DataServerApp::getInstance()->getPlayerMgr()->getPlayerByUserUID(nUserID);
		if (pPlayer == nullptr || pPlayer->isState(CPlayer::ePlayerState_Offline)) {
			vInvalid.push_back(nUserID);
			continue;
		}
		pPlayer->sendMsgToClient(jsMsg, MSG_CLUB_REAL_TIME_INFORMATION);
	}

	for (auto nInvalidUID : vInvalid) {
		removeFromIRT(nInvalidUID);
	}
}