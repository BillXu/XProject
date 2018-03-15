#include "LeagueEvent.h"
#include "log4z.h"
#include "League.h"
#include "LeagueMemberData.h"
#include "LeagueManager.h"
#include "ISeverApp.h"
#include "AsyncRequestQuene.h"
#include <time.h>
CLeagueEvent::CLeagueEvent() {
	m_eType = eLeagueComponent_Event;
	m_mAllEvents.clear();
	m_vDirtyIDs.clear();
	m_vAddIDs.clear();
	m_bReadingDB = false;
}

CLeagueEvent::~CLeagueEvent() {

}

void CLeagueEvent::init(CLeague* pLeague) {
	ILeagueComponent::init(pLeague);
	readEventFormDB();
}

void CLeagueEvent::reset() {
	m_bReadingDB = false;
	m_mAllEvents.clear();
	m_vDirtyIDs.clear();
	m_vAddIDs.clear();
}

bool CLeagueEvent::onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort, uint32_t nSenderID) {
	if (MSG_LEAGUE_EVENT_ACTIVE_LIST_UPDATE == nmsgType) {
		Json::Value jsMsg;
		jsMsg["leagueID"] = getLeague()->getLeagueID();
		if (recvValue["uid"].isNull() || recvValue["uid"].isUInt() == false) {
			//Json::Value jsMsg;
			jsMsg["ret"] = 1;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			return true;
		}
		auto nClubID = recvValue["clubID"].asUInt();
		jsMsg["clubID"] = nClubID;
		auto nUserID = recvValue["uid"].asUInt();
		if (getLeague()->getLeagueMemberData()->isNotJoin(nClubID)) {
			//Json::Value jsMsg;
			jsMsg["ret"] = 2;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			return true;
		}

		if (getLeague()->getLeagueMemberData()->checkUpdateLevel(nClubID, getEventLevel(eLeagueEventType_AppcationJoin)) == false) {
			//Json::Value jsMsg;
			jsMsg["ret"] = 3;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			return true;
		}
		jsMsg["ret"] = 0;
		Json::Value jsDetail, jsJoinInfo;
		uint32_t nJoinAmount(0);
		jsJoinInfo["type"] = eLeagueEventType_AppcationJoin;
		for (auto ref : m_mAllEvents) {
			if (ref.second.nState == eClubEventState_Wait) {
				if (ref.second.nEventType == eLeagueEventType_AppcationJoin) {
					nJoinAmount++;
				}
			}
		}
		jsJoinInfo["amount"] = nJoinAmount;
		jsDetail[jsDetail.size()] = jsJoinInfo;
		jsMsg["detail"] = jsDetail;
		sendMsgToClient(jsMsg, nmsgType, nSenderID);
		return true;
	}

	if (MSG_LEAGUE_QUIT_LEAGUE == nmsgType) {
		if (recvValue["clubID"].isNull() || recvValue["clubID"].isUInt() == false) {
			LOGFMTE("can not find clubID to quit league id = %u from session id = %u", getLeague()->getLeagueID(), nSenderID);
			Json::Value jsMsg;
			jsMsg["ret"] = 1;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			return true;
		}
		auto nClubID = recvValue["clubID"].asUInt();

		if (recvValue["uid"].isNull() || recvValue["uid"].isUInt() == false) {
			LOGFMTE("can not find uid to quit league id = %u from club id = %u", getLeague()->getLeagueID(), nClubID);
			Json::Value jsMsg;
			jsMsg["ret"] = 2;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			return true;
		}
		auto nUserID = recvValue["uid"].asUInt();

		Json::Value jsReq;
		jsReq["uid"] = nUserID;
		jsReq["leagueID"] = getLeague()->getLeagueID();
		jsReq["clubID"] = nClubID;
		auto pAsync = getLeague()->getLeagueMgr()->getSvrApp()->getAsynReqQueue();
		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nClubID, eAsync_league_ClubQuit_Check, jsReq, [recvValue, pAsync, nSenderID, this, nUserID, nClubID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
		{
			if (isTimeOut)
			{
				LOGFMTE(" request time out uid = %u , can not quit league id = %u by club id = %u", nUserID, getLeague()->getLeagueID(), nClubID);
				Json::Value jsRet;
				jsRet["ret"] = 7;
				sendMsgToClient(jsRet, MSG_LEAGUE_QUIT_LEAGUE, nSenderID);
				return;
			}

			uint8_t nReqRet = retContent["ret"].asUInt();
			uint8_t nRet = 0;
			do
			{
				if (0 != nReqRet)
				{
					nRet = 3;
					break;
				}

				if (getLeague()->getLeagueMemberData()->onClubQuit(nClubID)) {
					stEventData sted;
					sted.nEventID = getLeague()->getLeagueMgr()->generateEventID();
					sted.nDisposerUID = nUserID;
					sted.nState = eClubEventState_Accede;
					sted.nPostTime = time(NULL);
					sted.nEventType = eLeagueEventType_ClubQuit;
					sted.nLevel = getEventLevel(eLeagueEventType_ClubQuit);
					sted.jsDetail = recvValue;
					m_mAllEvents[sted.nEventID] = sted;
					m_vAddIDs.push_back(sted.nEventID);
					Json::Value jsMsg;
					jsMsg["leagueID"] = getLeague()->getLeagueID();
					jsMsg["uid"] = nUserID;
					jsMsg["clubID"] = nClubID;
					pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nClubID, eAsync_league_ClubQuit, jsMsg);
				}
				else {
					nRet = 5;
					break;
				}

			} while (0);

			Json::Value jsRet;
			jsRet["ret"] = nRet;
			sendMsgToClient(jsRet, MSG_LEAGUE_QUIT_LEAGUE, nSenderID);
			LOGFMTE("User(%u) from Club(%u) quit league(%u), ret = %u", nUserID, nClubID, getLeague()->getLeagueID(), nRet);
		}, recvValue);
		return true;
	}

	if (MSG_LEAGUE_DISMISS_LEAGUE == nmsgType) {
		if (recvValue["clubID"].isNull() || recvValue["clubID"].isUInt() == false) {
			LOGFMTE("can not find clubID to dismiss league id = %u from session id = %u", getLeague()->getLeagueID(), nSenderID);
			Json::Value jsMsg;
			jsMsg["ret"] = 1;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			return true;
		}
		auto nClubID = recvValue["clubID"].asUInt();

		if (recvValue["uid"].isNull() || recvValue["uid"].isUInt() == false) {
			LOGFMTE("can not find uid to dismiss league id = %u from club id = %u", getLeague()->getLeagueID(), nClubID);
			Json::Value jsMsg;
			jsMsg["ret"] = 2;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			return true;
		}
		auto nUserID = recvValue["uid"].asUInt();

		Json::Value jsReq;
		jsReq["uid"] = nUserID;
		jsReq["leagueID"] = getLeague()->getLeagueID();
		jsReq["clubID"] = nClubID;
		auto pAsync = getLeague()->getLeagueMgr()->getSvrApp()->getAsynReqQueue();
		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nClubID, eAsync_league_Dismiss_Check, jsReq, [recvValue, pAsync, nSenderID, this, nUserID, nClubID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
		{
			if (isTimeOut)
			{
				LOGFMTE(" request time out uid = %u , can not dismiss league id = %u from club id = %u", nUserID, getLeague()->getLeagueID(), nClubID);
				Json::Value jsRet;
				jsRet["ret"] = 7;
				sendMsgToClient(jsRet, MSG_LEAGUE_DISMISS_LEAGUE, nSenderID);
				return;
			}

			uint8_t nReqRet = retContent["ret"].asUInt();
			uint8_t nRet = 0;
			do
			{
				if (0 != nReqRet)
				{
					nRet = 3;
					break;
				}

				if (getLeague()->dismissLeague(nClubID)) {
					stEventData sted;
					sted.nEventID = getLeague()->getLeagueMgr()->generateEventID();
					sted.nDisposerUID = nUserID;
					sted.nState = eClubEventState_Accede;
					sted.nPostTime = time(NULL);
					sted.nEventType = eLeagueEventType_DismissLeague;
					sted.nLevel = getEventLevel(eLeagueEventType_DismissLeague);
					sted.jsDetail = recvValue;
					m_mAllEvents[sted.nEventID] = sted;
					m_vAddIDs.push_back(sted.nEventID);
					Json::Value jsMsg;
					jsMsg["leagueID"] = getLeague()->getLeagueID();
					jsMsg["uid"] = nUserID;
					getLeague()->getLeagueMemberData()->pushAsyncRequestToAll(ID_MSG_PORT_DATA, eAsync_league_Dismiss, jsMsg);
				}
				else {
					nRet = 5;
					break;
				}

			} while (0);

			Json::Value jsRet;
			jsRet["ret"] = nRet;
			sendMsgToClient(jsRet, MSG_LEAGUE_DISMISS_LEAGUE, nSenderID);
			LOGFMTE("User(%u) from Club(%u) dismiss league(%u), ret = %u", nUserID, nClubID, getLeague()->getLeagueID(), nRet);
		}, recvValue);
		return true;
	}

	if (MSG_LEAGUE_FIRE_CLUB == nmsgType) {
		if (recvValue["clubID"].isNull() || recvValue["clubID"].isUInt() == false) {
			LOGFMTE("can not find clubID to fire club form league id = %u from session id = %u", getLeague()->getLeagueID(), nSenderID);
			Json::Value jsMsg;
			jsMsg["ret"] = 1;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			return true;
		}
		auto nClubID = recvValue["clubID"].asUInt();
		if (getLeague()->getLeagueMemberData()->checkUpdateLevel(nClubID, getEventLevel(eLeagueEventType_FireClub)) == false) {
			LOGFMTE("clubID do not have enough level to fire club form league id = %u from session id = %u", getLeague()->getLeagueID(), nSenderID);
			Json::Value jsMsg;
			jsMsg["ret"] = 6;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			return true;
		}

		if (recvValue["uid"].isNull() || recvValue["uid"].isUInt() == false) {
			LOGFMTE("can not find clubID to fire club form league id = %u from session id = %u", getLeague()->getLeagueID(), nSenderID);
			Json::Value jsMsg;
			jsMsg["ret"] = 2;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			return true;
		}
		auto nUserID = recvValue["uid"].asUInt();
		if (recvValue["fireCID"].isNull() || recvValue["fireCID"].isUInt() == false) {
			LOGFMTE("can not find fireID to fire club form league id = %u from user id = %u", getLeague()->getLeagueID(), nUserID);
			Json::Value jsMsg;
			jsMsg["ret"] = 3;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			return true;
		}
		auto nFireCID = recvValue["fireCID"].asUInt();
		Json::Value jsReq;
		jsReq["uid"] = nUserID;
		jsReq["leagueID"] = getLeague()->getLeagueID();
		jsReq["clubID"] = nClubID;
		auto pAsync = getLeague()->getLeagueMgr()->getSvrApp()->getAsynReqQueue();
		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nClubID, eAsync_league_FireClub_Check, jsReq, [recvValue, pAsync, nSenderID, this, nUserID, nClubID, nFireCID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
		{
			if (isTimeOut)
			{
				LOGFMTE(" request time out uid = %u , can not fire club id = %u from league id = %u ", nUserID, nClubID, getLeague()->getLeagueID());
				Json::Value jsRet;
				jsRet["ret"] = 7;
				sendMsgToClient(jsRet, MSG_LEAGUE_FIRE_CLUB, nSenderID);
				return;
			}

			uint8_t nReqRet = retContent["ret"].asUInt();
			uint8_t nRet = 0;
			do
			{
				if (0 != nReqRet)
				{
					nRet = 4;
					break;
				}

				if (getLeague()->getLeagueMemberData()->fireClub(nClubID, nFireCID)) {
					stEventData sted;
					sted.nEventID = getLeague()->getLeagueMgr()->generateEventID();
					sted.nDisposerUID = nUserID;
					sted.nState = eClubEventState_Accede;
					sted.nPostTime = time(NULL);
					sted.nEventType = eLeagueEventType_FireClub;
					sted.nLevel = getEventLevel(eLeagueEventType_FireClub);
					sted.jsDetail = recvValue;
					m_mAllEvents[sted.nEventID] = sted;
					m_vAddIDs.push_back(sted.nEventID);
					Json::Value jsMsg;
					jsMsg["leagueID"] = getLeague()->getLeagueID();
					jsMsg["clubID"] = nFireCID;
					jsMsg["agentCID"] = nClubID;
					jsMsg["uid"] = nUserID;
					pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nFireCID, eAsync_league_FireClub, jsMsg);
				}
				else {
					nRet = 5;
					break;
				}

			} while (0);

			Json::Value jsRet;
			jsRet["ret"] = nRet;
			sendMsgToClient(jsRet, MSG_LEAGUE_FIRE_CLUB, nSenderID);
			LOGFMTE("User(%u) from Club(%u) fire Club(%u) from league(%u), ret = %u", nUserID, nClubID, nFireCID, getLeague()->getLeagueID(), nRet);
		}, recvValue);

		return true;
	}

	if (MSG_LEAGUE_JOIN_LEAGUE == nmsgType) {
		if (getLeague()->getJoinLimit()) {
			LOGFMTE("can not join league id = %u league now is limit join", getLeague()->getLeagueID());
			Json::Value jsMsg;
			jsMsg["ret"] = 6;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			return true;
		}

		if (recvValue["clubID"].isNull() || recvValue["clubID"].isUInt() == false) {
			Json::Value jsMsg;
			jsMsg["ret"] = 1;
			LOGFMTE("can not find clubID to join league id = %u from session id = %u", getLeague()->getLeagueID(), nSenderID);
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			return true;
		}
		auto nClubID = recvValue["clubID"].asUInt();

		if (recvValue["uid"].isNull() || recvValue["uid"].isUInt() == false) {
			Json::Value jsMsg;
			jsMsg["ret"] = 2;
			LOGFMTE("can not find uid to join league id = %u, club id = %u, from session id = %d", getLeague()->getLeagueID(), nClubID, nSenderID);
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			return true;
		}
		auto nUserID = recvValue["uid"].asUInt();

		if (hasApplayJoin(nClubID)) {
			Json::Value jsMsg;
			LOGFMTE("club apply join to league error, is already apply join, uid = %u, clubID = %u, leagueID = %u", nUserID, nClubID, getLeague()->getLeagueID());
			jsMsg["ret"] = 3;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			return true;
		}

		Json::Value jsReq;
		jsReq["uid"] = nUserID;
		jsReq["leagueID"] = getLeague()->getLeagueID();
		jsReq["clubID"] = nClubID;
		auto pAsync = getLeague()->getLeagueMgr()->getSvrApp()->getAsynReqQueue();
		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nClubID, eAsync_league_JoinLeague, jsReq, [recvValue, nSenderID, this, nUserID, nClubID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
		{
			if (isTimeOut)
			{
				LOGFMTE(" request time out uid = %u , can not join league id = %u ", nUserID, getLeague()->getLeagueID());
				Json::Value jsRet;
				jsRet["ret"] = 7;
				sendMsgToClient(jsRet, MSG_LEAGUE_JOIN_LEAGUE, nSenderID);
				return;
			}

			uint8_t nReqRet = retContent["ret"].asUInt();
			uint8_t nRet = 0;
			do
			{
				if (0 != nReqRet)
				{
					nRet = 4;
					break;
				}

				stEventData sted;
				sted.nEventID = getLeague()->getLeagueMgr()->generateEventID();
				sted.nDisposerUID = 0;
				sted.nState = eClubEventState_Wait;
				sted.nPostTime = time(NULL);
				sted.nEventType = eLeagueEventType_AppcationJoin;
				sted.nLevel = getEventLevel(eLeagueEventType_AppcationJoin);
				sted.jsDetail = recvValue;
				m_mAllEvents[sted.nEventID] = sted;
				m_vAddIDs.push_back(sted.nEventID);

				Json::Value jsMsg;
				jsMsg["leagueID"] = getLeague()->getLeagueID();
				jsMsg["type"] = eLeagueEventType_AppcationJoin;
				getLeague()->getLeagueMemberData()->pushAsyncRequestToLevelNeed(ID_MSG_PORT_DATA, eAsync_club_League_Push_Event, jsMsg, sted.nLevel);

			} while (0);

			Json::Value jsRet;
			jsRet["ret"] = nRet;
			sendMsgToClient(jsRet, MSG_LEAGUE_JOIN_LEAGUE, nSenderID);
			LOGFMTE("User(%u) from Club(%u) apply join to league(%u), ret = %u", nUserID, nClubID, getLeague()->getLeagueID(), nRet);
		}, recvValue);
		return true;
	}

	if (MSG_LEAGUE_EVENT_JOIN == nmsgType) {
		Json::Value jsMsg, jsEvents;
		for (auto& ref : m_mAllEvents) {
			auto& data = ref.second;
			if (data.nEventType == eLeagueEventType_AppcationJoin) {
				Json::Value jsEvent;
				jsEvent["eventID"] = data.nEventID;
				jsEvent["time"] = data.nPostTime;
				jsEvent["state"] = data.nState;
				jsEvent["disposer"] = data.nDisposerUID;
				jsEvent["detail"] = data.jsDetail;
				jsEvents[jsEvents.size()] = jsEvent;
			}
		}
		jsMsg["ret"] = 0;
		jsMsg["events"] = jsEvents;
		sendMsgToClient(jsMsg, nmsgType, nSenderID);
		return true;
	}

	if (MSG_LEAGUE_EVENT_APPLY_TREAT == nmsgType) {
		uint32_t nPlayerID = recvValue["uid"].isUInt() ? recvValue["uid"].asUInt() : 0;
		Json::Value jsResult;
		if (nPlayerID == 0) {
			LOGFMTE("User apply treat league event error, uid is missing, sessionID = %u, leagueID = %u", nSenderID, getLeague()->getLeagueID());
			jsResult["ret"] = 1;
			sendMsgToClient(jsResult, nmsgType, nSenderID);
			return true;
		}

		uint32_t nEventID = recvValue["eventID"].isUInt() ? recvValue["eventID"].asUInt() : 0;
		if (nEventID == 0) {
			LOGFMTE("User apply treat league event error, eventID is missing, playerID = %u, leagueID = %u", nPlayerID, getLeague()->getLeagueID());
			jsResult["ret"] = 2;
			sendMsgToClient(jsResult, nmsgType, nSenderID);
			return true;
		}

		uint32_t nClubID = recvValue["clubID"].isUInt() ? recvValue["clubID"].asUInt() : 0;
		if (nClubID == 0) {
			LOGFMTE("User apply treat league event error, clubID is missing, nEventID = %u, playerID = %u, leagueID = %u", nEventID, nPlayerID, getLeague()->getLeagueID());
			jsResult["ret"] = 3;
			sendMsgToClient(jsResult, nmsgType, nSenderID);
			return true;
		}

		if (recvValue["state"].isNull() || recvValue["state"].isUInt() == false) {
			LOGFMTE("User apply treat league event error, state is missing, playerID = %u, leagueID = %u", nPlayerID, getLeague()->getLeagueID());
			jsResult["ret"] = 4;
			sendMsgToClient(jsResult, nmsgType, nSenderID);
			return true;
		}
		uint8_t nState = recvValue["state"].asUInt();
		uint8_t nTreatLevel = getEventTreatLevel(nEventID);
		if (nTreatLevel == 0) {
			LOGFMTE("User apply treat league event error, event ID or type is wrong, playerID = %u, leagueID = %u, eventID = %u", nPlayerID, getLeague()->getLeagueID(), nEventID);
			jsResult["ret"] = 5;
			sendMsgToClient(jsResult, nmsgType, nSenderID);
			return true;
		}

		Json::Value jsReq;
		jsReq["uid"] = nPlayerID;
		jsReq["leagueID"] = getLeague()->getLeagueID();
		jsReq["level"] = nTreatLevel;
		jsReq["clubID"] = nClubID;
		auto pAsync = getLeague()->getLeagueMgr()->getSvrApp()->getAsynReqQueue();
		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nClubID, eAsync_league_TreatEvent, jsReq, [recvValue, nSenderID, this, nPlayerID, nClubID, nEventID, nState](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
		{
			if (isTimeOut)
			{
				LOGFMTE(" request time out uid = %u from clubID = %u , can not treat league event leagueID = %u, eventID = %u ", nPlayerID, nClubID, getLeague()->getLeagueID(), nEventID);
				Json::Value jsRet;
				jsRet["ret"] = 7;
				sendMsgToClient(jsRet, MSG_LEAGUE_EVENT_APPLY_TREAT, nSenderID);
				return;
			}

			uint8_t nReqRet = retContent["ret"].asUInt();
			uint8_t nRet = 0;
			do
			{
				if (0 != nReqRet)
				{
					nRet = 6;
					break;
				}
				nRet = treatEvent(nEventID, nClubID, nPlayerID, nState);

			} while (0);

			Json::Value jsRet;
			jsRet["ret"] = nRet;
			sendMsgToClient(jsRet, MSG_LEAGUE_EVENT_APPLY_TREAT, nSenderID);
			LOGFMTE("User(%u) from Club(%u) teat league event of league(%u), ret = %u", nPlayerID, nClubID, getLeague()->getLeagueID(), nRet);
		}, recvValue);

		return true;

		/*uint8_t nRet = treatEvent(nEventID, nPlayerID, nState, nSenderID);
		jsResult["ret"] = nRet;
		sendMsgToClient(jsResult, nmsgType, nSenderID);
		return true;*/
		/*auto& itEvent = m_mAllEvents.find(nEventID);
		if (itEvent == m_mAllEvents.end()) {
		LOGFMTE("User apply treat club event error, can not find event, eventID = %u, playerID = %u, clubID = %u", nEventID, nPlayerID, getClub()->getClubID());
		jsResult["ret"] = 3;
		sendMsgToClient(jsResult, nmsgType, nSenderID);
		return true;
		}

		if (itEvent->second.nState != eClubEventState_Wait) {
		LOGFMTE("User apply treat club event error, event is already be treated, eventID = %u, playerID = %u, clubID = %u", nEventID, nPlayerID, getClub()->getClubID());
		jsResult["ret"] = 4;
		sendMsgToClient(jsResult, nmsgType, nSenderID);
		return true;
		}*/
	}

	return false;
}

bool CLeagueEvent::onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult) {
	/*if (eAsync_club_agree_DragIn == nRequestType) {
		auto nClubID = jsReqContent["clubID"].asUInt();
		if (getLeague()->getLeagueMemberData()->isNotJoin(nClubID)) {
			jsResult["ret"] = 1;
			return true;
		}
		auto nAmount = jsReqContent["amount"].asUInt();
		if (getLeague()->getLeagueMemberData()->checkDecreaseIntegration(nClubID, nAmount) == false) {
			jsResult["ret"] = 11;
			return true;
		}

		return true;
	}*/

	return false;
}

bool CLeagueEvent::onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID) {
	if (eAsync_club_agree_DragIn == nRequestType) {
		auto pApp = getLeague()->getLeagueMgr()->getSvrApp();
		auto nClubID = jsReqContent["clubID"].asUInt();
		/*if (getLeague()->getLeagueMemberData()->isNotJoin(nClubID)) {
			Json::Value jsRet;
			jsRet["ret"] = 11;
			pApp->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsRet, getLeague()->getLeagueID());
			return true;
		}*/
		auto nAmount = jsReqContent["amount"].asUInt();
		if (getLeague()->getLeagueMemberData()->checkDecreaseIntegration(nClubID, nAmount) == false) {
			Json::Value jsRet;
			jsRet["ret"] = 11;
			pApp->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsRet, getLeague()->getLeagueID());
			return true;
		}
		auto nPort = jsReqContent["port"].asUInt();
		auto nRoomID = jsReqContent["roomID"].asUInt();
		auto nUID = jsReqContent["uid"].asUInt();
		Json::Value jsMsg;
		jsMsg["amount"] = nAmount;
		jsMsg["roomID"] = nRoomID;
		jsMsg["uid"] = nUID;
		jsMsg["clubID"] = nClubID;
		pApp->getAsynReqQueue()->pushAsyncRequest(nPort, nRoomID, eAsync_club_agree_DragIn, jsMsg, [this, nRoomID, nClubID, nSenderID, nSenderPort, nReqSerial, nAmount, pApp](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut) {
			Json::Value jsRet;
			if (isTimeOut) {
				getLeague()->getLeagueMemberData()->clearTempIntegration(nClubID, nAmount);
				jsRet["ret"] = 7;
				pApp->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsRet, getLeague()->getLeagueID());
				return;
			}

			if (retContent["ret"].asUInt()) {
				getLeague()->getLeagueMemberData()->clearTempIntegration(nClubID, nAmount);
				jsRet["ret"] = 13;
				pApp->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsRet, getLeague()->getLeagueID());
				return;
			}

			getLeague()->getLeagueMemberData()->decreaseIntegration(nClubID, nAmount);
			jsRet["ret"] = 0;
			pApp->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsRet, getLeague()->getLeagueID());

			stEventData sted;
			sted.nEventID = getLeague()->getLeagueMgr()->generateEventID();
			sted.nDisposerUID = 0;
			sted.nState = eClubEventState_Accede;
			sted.nPostTime = time(NULL);
			sted.nEventType = eLeagueEventType_ClubEntry;
			sted.nLevel = getEventLevel(eLeagueEventType_ClubEntry);
			sted.jsDetail = jsUserData;
			m_mAllEvents[sted.nEventID] = sted;
			m_vAddIDs.push_back(sted.nEventID);
		}, jsMsg);

		return true;
	}
	
	return false;
}

void CLeagueEvent::joinEventWaitToJson(Json::Value& jsMsg) {
	for (auto& ref : m_mAllEvents) {
		auto& data = ref.second;
		if (data.nEventType == eLeagueEventType_AppcationJoin && data.nState == eClubEventState_Wait) {
			Json::Value jsEvent;
			jsEvent["eventID"] = data.nEventID;
			jsEvent["time"] = data.nPostTime;
			jsEvent["detail"] = data.jsDetail;
			jsMsg[jsMsg.size()] = jsEvent;
		}
	}
}

void CLeagueEvent::timerSave() {
	if (m_bReadingDB) {
		return;
	}

	if (m_vDirtyIDs.empty() && m_vAddIDs.empty()) {
		return;
	}

	for (auto nAddUID : m_vAddIDs) {
		if (m_mAllEvents.find(nAddUID) == m_mAllEvents.end()) {
			LOGFMTE("caution: add league event error, can not find event info, leagueID = %u , eventID = %u, caution!!!!!", getLeague()->getLeagueID(), nAddUID);
			continue;
		}
		auto info = m_mAllEvents[nAddUID];

		Json::StyledWriter ss;
		auto jsDetail = ss.write(info.jsDetail);
		Json::Value jssql;
		char pBuffer[512] = { 0 };
		if (info.nDisposerUID) {
			sprintf_s(pBuffer, "insert into leagueevent (eventID, leagueID, postTime, eventType, level, state, disposerUID, detail) values (%u, %u, from_unixtime( %u ), %u, %u, %u, %u,", info.nEventID, getLeague()->getLeagueID(), info.nPostTime, info.nEventType, info.nLevel, info.nState, info.nDisposerUID);
		}
		else {
			sprintf_s(pBuffer, "insert into clubevent (eventID, leagueID, postTime, eventType, level, state, detail) values (%u, %u, from_unixtime( %u ), %u, %u, %u,", info.nEventID, getLeague()->getLeagueID(), info.nPostTime, info.nEventType, info.nLevel, info.nState);
		}
		//sprintf_s(pBuffer, "insert into clubevent (eventID, clubID, postTime, eventType, level, state, detail) values (%u, %u, %u, %u, %u, %u,", info.nEventID, getClub()->getClubID(), info.nPostTime, info.nEventType, info.nLevel, info.nState);
		std::ostringstream ssSql;
		ssSql << pBuffer << " ' " << jsDetail << " ' );";
		jssql["sql"] = ssSql.str();
		auto pReqQueue = m_pLeague->getLeagueMgr()->getSvrApp()->getAsynReqQueue();
		pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, getLeague()->getLeagueID(), eAsync_DB_Add, jssql);
	}
	m_vAddIDs.clear();

	for (auto nDirtyUID : m_vDirtyIDs) {
		if (m_mAllEvents.find(nDirtyUID) == m_mAllEvents.end()) {
			LOGFMTE("caution: save league event error, can not find event info, leagueID = %u , eventID = %u, caution!!!!!", getLeague()->getLeagueID(), nDirtyUID);
			continue;
		}
		auto info = m_mAllEvents[nDirtyUID];
		Json::Value jssql;
		char pBuffer[512] = { 0 };
		if (info.nDisposerUID) {
			sprintf_s(pBuffer, "update leagueevent set state = %u, disposerUID = %u where eventID = %u;", info.nState, info.nDisposerUID, nDirtyUID);
		}
		else {
			sprintf_s(pBuffer, "update clubevent set state = %u where eventID = %u;", info.nState, nDirtyUID);
		}

		std::string str = pBuffer;
		jssql["sql"] = pBuffer;
		auto pReqQueue = m_pLeague->getLeagueMgr()->getSvrApp()->getAsynReqQueue();
		pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, getLeague()->getLeagueID(), eAsync_DB_Update, jssql);
	}
	m_vDirtyIDs.clear();

	for (auto it = m_mAllEvents.begin(); it != m_mAllEvents.end();) {
		if (it->second.nPostTime < time(NULL) - AUTO_TREET_EVENT_TIME && it->second.nState == eClubEventState_Wait) {
			it->second.nState = eClubEventState_Decline;
			m_vDirtyIDs.push_back(it->first);
		}

		if (it->second.nPostTime < time(NULL) - AUTO_RELEASE_EVENT_TIME && eventIsDirty(it->first) == false) {
			m_mAllEvents.erase(it++);
		}
		else {
			it++;
		}
	}
}

void CLeagueEvent::readEventFormDB(uint32_t nOffset) {
	m_bReadingDB = true;
	std::ostringstream ss;
	ss << "SELECT eventID, unix_timestamp(postTime) as postTime, eventType, level, state, disposerUID, detail FROM leagueevent where leagueID = " << m_pLeague->getLeagueID() << " order by eventID desc limit 10 offset " << (UINT)nOffset << ";";
	Json::Value jsReq;
	jsReq["sql"] = ss.str();
	m_pLeague->getLeagueMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB, rand(), eAsync_DB_Select, jsReq, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut) {
		if (isTimeOut)
		{
			LOGFMTE("caution: load league event from DB time out, leagueID = %u , caution!!!!!", getLeague()->getLeagueID());
			doProcessAfterReadDB();
			return;
		}

		uint32_t nAft = retContent["afctRow"].asUInt();
		auto jsData = retContent["data"];
		if (nAft == 0 || jsData.isNull())
		{
			doProcessAfterReadDB();
			return;
		}

		for (uint32_t nRowIdx = 0; nRowIdx < nAft; ++nRowIdx)
		{
			auto jsRow = jsData[nRowIdx];
			auto nEventID = jsRow["eventID"].asUInt();
			auto iter = m_mAllEvents.find(nEventID);
			if (iter != m_mAllEvents.end())
			{
				LOGFMTE("bug: why already have this league event eventID = %u , in league id = %u double read , bug!!!!!!", nEventID, getLeague()->getLeagueID());
				doProcessAfterReadDB();
				return;
			}

			stEventData stEventInfo;
			stEventInfo.nEventID = nEventID;
			stEventInfo.nPostTime = jsRow["postTime"].asUInt();
			stEventInfo.nEventType = jsRow["eventType"].asUInt();
			stEventInfo.nState = jsRow["state"].asUInt();
			stEventInfo.nLevel = jsRow["level"].asUInt();
			stEventInfo.nDisposerUID = jsRow["disposerUID"].asUInt();

			Json::Value jsDetail;
			Json::Reader jsReader;
			jsReader.parse(jsRow["detail"].asString(), jsDetail, false);
			stEventInfo.jsDetail = jsDetail;

			m_mAllEvents[nEventID] = stEventInfo;
		}

		if (nAft < 10)  // only read one page ;
		{
			doProcessAfterReadDB();
			return;
		}

		// not finish , go on read 
		auto nSize = m_mAllEvents.size();
		readEventFormDB(nSize);
	});
}

void CLeagueEvent::doProcessAfterReadDB() {
	m_bReadingDB = false;
}

uint8_t CLeagueEvent::getEventLevel(uint8_t nEventType) {
	switch (nEventType)
	{
	case eLeagueEventType_AppcationJoin:
	{
		return eLeagueMemberLevel_Creator;
	}
	case eLeagueEventType_FireClub:
	{
		return eLeagueMemberLevel_Creator;
	}
	case eLeagueEventType_DismissLeague:
	{
		return eLeagueMemberLevel_Creator;
	}
	case eLeagueEventType_ClubQuit:
	{
		return eClubMemberLevel_None;
	}
	default:
	{
		return eClubMemberLevel_None;
	}
	}
}

uint8_t CLeagueEvent::getEventTreatLevel(uint32_t nEventID) {
	if (m_mAllEvents.count(nEventID)) {
		auto nType = m_mAllEvents[nEventID].nEventType;
		switch (nType)
		{
		case eLeagueEventType_AppcationJoin:
		{
			return eClubMemberLevel_Admin;
		}
		default:
		{
			return 0;
		}
		}
	}
	return 0;
}

bool CLeagueEvent::eventIsDirty(uint32_t nEventID) {
	return std::find(m_vDirtyIDs.begin(), m_vDirtyIDs.end(), nEventID) == m_vDirtyIDs.end() &&
		std::find(m_vAddIDs.begin(), m_vAddIDs.end(), nEventID) == m_vAddIDs.end();
}

uint8_t CLeagueEvent::treatEvent(uint32_t nEventID, uint32_t nClubID, uint32_t nPlayerID, uint8_t nState) {
	auto& itEvent = m_mAllEvents.find(nEventID);
	if (itEvent == m_mAllEvents.end()) {
		LOGFMTE("User apply treat league event error, can not find event, eventID = %u, playerID = %u, leagueID = %u", nEventID, nPlayerID, getLeague()->getLeagueID());
		return 8;
	}

	if (itEvent->second.nState != eClubEventState_Wait) {
		LOGFMTE("User apply treat club event error, event is already be treated, eventID = %u, playerID = %u, leagueID = %u", nEventID, nPlayerID, getLeague()->getLeagueID());
		return 9;
	}

	if (getLeague()->getLeagueMemberData()->checkUpdateLevel(nClubID, itEvent->second.nLevel) == false) {
		LOGFMTE("User apply treat league event error, club level is not enough, eventID = %u, playerID = %u, clubID = %u, leagueID = %u", nEventID, nPlayerID, nClubID, getLeague()->getLeagueID());
		return 10;
	}

	if (nState == eClubEventState_Accede) {
		switch (itEvent->second.nEventType) {
		case eClubEventType_AppcationJoin: {
			uint32_t nMemberCID = itEvent->second.jsDetail["clubID"].asUInt();
			if (getLeague()->getLeagueMemberData()->addMember(nMemberCID) == false) {
				LOGFMTE("User apply treat league event error, league refused could be full? eventID = %u, playerID = %u, leagueID = %u", nEventID, nPlayerID, getLeague()->getLeagueID());
				return 11;
			}
			Json::Value jsMsg;
			jsMsg["leagueID"] = getLeague()->getLeagueID();
			jsMsg["agentID"] = nPlayerID;
			jsMsg["clubID"] = nMemberCID;
			getLeague()->getLeagueMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, nMemberCID, eAsync_league_ClubJoin, jsMsg);
			itEvent->second.nState = eClubEventState_Accede;
			break;
		}
		default: {
			LOGFMTE("User apply treat league event error, event type can not be treated, eventID = %u, playerID = %u, leagueID = %u", nEventID, nPlayerID, getLeague()->getLeagueID());
			return 12;
		}
		}
	}
	else {
		itEvent->second.nState = eClubEventState_Decline;
	}

	itEvent->second.nDisposerUID = nPlayerID;
	m_vDirtyIDs.push_back(itEvent->first);
	return 0;
}

bool CLeagueEvent::hasApplayJoin(uint32_t nClubID) {
	for (auto ref : m_mAllEvents) {
		if (ref.second.nEventType == eLeagueEventType_AppcationJoin && ref.second.nState == eClubEventState_Wait) {
			if (ref.second.jsDetail["clubID"].asUInt() == nClubID) {
				return true;
			}
		}
	}
	return false;
}