#include "ClubEvent.h"
#include "Club.h"
#include "log4z.h"
#include "ClubManager.h"
#include "ISeverApp.h"
#include "AsyncRequestQuene.h"
#include "ClubMemberData.h"
#include "ClubGameData.h"
#include <time.h>
#define ENTRY_RECORDER_LIST_LIMIT 100

CClubEvent::CClubEvent() {
	m_eType = eClubComponent_Event;
	m_mAllEvents.clear();
	m_vDirtyIDs.clear();
	m_vAddIDs.clear();
	m_bReadingDB = false;
}

CClubEvent::~CClubEvent() {

}

void CClubEvent::init(CClub* pClub) {
	IClubComponent::init(pClub);
	readEventFormDB();
}

void CClubEvent::reset() {
	m_bReadingDB = false;
	m_mAllEvents.clear();
	m_vDirtyIDs.clear();
	m_vAddIDs.clear();
}

bool CClubEvent::onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort, uint32_t nSenderID) {
	if (MSG_CLUB_SYSTEM_AUTO_ADD_PLAYER == nmsgType) {
		uint32_t nMemberID = recvValue["uid"].isUInt() ? recvValue["uid"].asUInt() : 0;
		auto cName = recvValue["name"].asCString();
		if (nMemberID == 0) {
			LOGFMTE("User apply auto join club error, userID is missing, sessionID = %u, clubID = %u", nSenderID, getClub()->getClubID());
			//Json::Value jsResult;
			//jsResult["ret"] = 1;
			//sendMsgToClient(jsResult, nmsgType, nSenderID);
			return true;
		}
		if (getClub()->getClubMemberData()->addMember(nMemberID, cName)) {
			stEventData sted;
			sted.nEventID = getClub()->getClubMgr()->generateEventID();
			sted.nDisposerUID = 0;
			sted.nState = eClubEventState_Accede;
			sted.nPostTime = time(NULL);
			sted.nEventType = eClubEventType_AppcationJoin;
			sted.nLevel = getEventLevel(eClubEventType_AppcationJoin);
			sted.jsDetail = recvValue;
			m_mAllEvents[sted.nEventID] = sted;
			m_vAddIDs.push_back(sted.nEventID);
			Json::Value jsMsg;
			jsMsg["targetUID"] = nMemberID;
			jsMsg["clubID"] = getClub()->getClubID();
			jsMsg["agentID"] = 0;
			getClub()->getClubMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, nMemberID, eAsync_Club_Join, jsMsg);
		}
		return true;
	}

	if (MSG_CLUB_EVENT_ENTRY_UPDATE == nmsgType) {
		Json::Value jsMsg;
		jsMsg["clubID"] = getClub()->getClubID();
		if (recvValue["uid"].isNull() || recvValue["uid"].isUInt() == false) {
			Json::Value jsMsg;
			jsMsg["ret"] = 1;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			return true;
		}
		auto nUserID = recvValue["uid"].asUInt();
		if (getClub()->getClubMemberData()->isNotJoin(nUserID)) {
			Json::Value jsMsg;
			jsMsg["ret"] = 2;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			return true;
		}

		if (getClub()->getClubMemberData()->checkUpdateLevel(nUserID, getEventLevel(eClubEventType_AppcationEntry)) == false) {
			Json::Value jsMsg;
			jsMsg["ret"] = 3;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			return true;
		}

		jsMsg["ret"] = 0;
		Json::Value jsDetail, jsJoinInfo, jsEntryInfo, jsMTTEntryInfo;
		uint32_t nJoinAmount(0), nEntryAmount(0), nMTTEntryAmount(0);
		jsJoinInfo["type"] = eClubEventType_AppcationJoin;
		jsEntryInfo["type"] = eClubEventType_AppcationEntry;
		jsMTTEntryInfo["type"] = eClubEventType_MTTAppcationEntry;
		for (auto ref : m_mAllEvents) {
			if (ref.second.nState == eClubEventState_Wait) {
				if (ref.second.nEventType == eClubEventType_AppcationJoin) {
					nJoinAmount++;
				}
				else if(ref.second.nEventType == eClubEventType_AppcationEntry){
					if (ref.second.jsDetail["mtt"].asBool()) {
						nMTTEntryAmount++;
					}
					else {
						nEntryAmount++;
					}
				}
			}
		}
		jsJoinInfo["amount"] = nJoinAmount;
		jsEntryInfo["amount"] = nEntryAmount;
		jsMTTEntryInfo["amount"] = nMTTEntryAmount;
		jsDetail[jsDetail.size()] = jsJoinInfo;
		jsDetail[jsDetail.size()] = jsEntryInfo;
		jsDetail[jsDetail.size()] = jsMTTEntryInfo;
		jsMsg["detail"] = jsDetail;
		sendMsgToClient(jsMsg, nmsgType, nSenderID);
		return true;
	}
	if (MSG_CLUB_EVENT_GRANT_RECORDER == nmsgType) {
		Json::Value jsMsg, jsEvents;
		jsMsg["ret"] = 0;
		jsMsg["clubID"] = getClub()->getClubID();
		for (auto ref : m_mAllEvents) {
			if (ref.second.nEventType == eClubEventType_GrantFoundation) {
				Json::Value jsEvent;
				jsEvent["eventID"] = ref.first;
				jsEvent["time"] = ref.second.nPostTime;
				jsEvent["disposer"] = ref.second.nDisposerUID;
				jsEvent["detail"] = ref.second.jsDetail;
				jsEvents[jsEvents.size()] = jsEvent;
			}
		}
		jsMsg["events"] = jsEvents;
		sendMsgToClient(jsMsg, nmsgType, nSenderID);
		return true;
	}

	if (MSG_CLUB_QUIT_CLUB == nmsgType) {
		uint32_t nMemberID = recvValue["uid"].isUInt() ? recvValue["uid"].asUInt() : 0;
		if (nMemberID == 0) {
			LOGFMTE("User apply quit club error, userID is missing, sessionID = %u, clubID = %u", nSenderID, getClub()->getClubID());
			Json::Value jsResult;
			jsResult["ret"] = 1;
			sendMsgToClient(jsResult, nmsgType, nSenderID);
			return true;
		}
		auto nLevel = getClub()->getClubMemberData()->getMemberLevel(nMemberID);
		if (nLevel && nLevel < eClubMemberLevel_Creator) {
			if (getClub()->getClubMemberData()->fireMember(nMemberID)) {
				stEventData sted;
				sted.nEventID = getClub()->getClubMgr()->generateEventID();
				sted.nDisposerUID = nMemberID;
				sted.nState = eClubEventState_Accede;
				sted.nPostTime = time(NULL);
				sted.nEventType = eClubEventType_PlayerQuit;
				sted.nLevel = getEventLevel(eClubEventType_PlayerQuit);
				sted.jsDetail = recvValue;
				m_mAllEvents[sted.nEventID] = sted;
				m_vAddIDs.push_back(sted.nEventID);
				Json::Value jsMsg;
				jsMsg["clubID"] = getClub()->getClubID();
				jsMsg["uid"] = nMemberID;
				jsMsg["targetUID"] = nMemberID;
				getClub()->getClubMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, nMemberID, eAsync_Club_Quit, jsMsg);
				LOGFMTE("User apply quit club complete, uid = %u, clubID = %u", nMemberID, getClub()->getClubID());
				Json::Value jsResult;
				jsResult["ret"] = 0;
				sendMsgToClient(jsResult, nmsgType, nSenderID);
			}
			else {
				LOGFMTE("User apply quit club error, memberData refused, uid = %u, clubID = %u", nMemberID, getClub()->getClubID());
				Json::Value jsResult;
				jsResult["ret"] = 3;
				sendMsgToClient(jsResult, nmsgType, nSenderID);
			}
		}
		else {
			LOGFMTE("User apply quit club error, uid is illegal, uid = %u, clubID = %u, level = %u", nMemberID, getClub()->getClubID(), nLevel);
			Json::Value jsResult;
			jsResult["ret"] = 2;
			sendMsgToClient(jsResult, nmsgType, nSenderID);
		}
		return true;
	}
	if (MSG_CLUB_FIRE_PLAYER == nmsgType) {
		uint32_t nMemberID = recvValue["uid"].isUInt() ? recvValue["uid"].asUInt() : 0;
		if (nMemberID == 0) {
			LOGFMTE("User apply fire player in club error, userID is missing, sessionID = %u, clubID = %u", nSenderID, getClub()->getClubID());
			Json::Value jsResult;
			jsResult["ret"] = 1;
			sendMsgToClient(jsResult, nmsgType, nSenderID);
			return true;
		}
		uint32_t nFireUID = recvValue["fireUID"].isUInt() ? recvValue["fireUID"].asUInt() : 0;
		if (nFireUID == 0) {
			LOGFMTE("User apply fire player in club error, fireUID is missing, uid = %u, clubID = %u", nMemberID, getClub()->getClubID());
			Json::Value jsResult;
			jsResult["ret"] = 2;
			sendMsgToClient(jsResult, nmsgType, nSenderID);
			return true;
		}

		auto nLevel = getClub()->getClubMemberData()->getMemberLevel(nMemberID);
		auto nFireLevel = getClub()->getClubMemberData()->getMemberLevel(nFireUID);
		if (nLevel && nFireLevel && nLevel > nFireLevel && nLevel >= getEventLevel(eClubEventType_FirePlayer)) {
			if (getClub()->getClubMemberData()->fireMember(nFireUID)) {
				stEventData sted;
				sted.nEventID = getClub()->getClubMgr()->generateEventID();
				sted.nDisposerUID = nMemberID;
				sted.nState = eClubEventState_Accede;
				sted.nPostTime = time(NULL);
				sted.nEventType = eClubEventType_FirePlayer;
				sted.nLevel = getEventLevel(eClubEventType_FirePlayer);
				sted.jsDetail = recvValue;
				m_mAllEvents[sted.nEventID] = sted;
				m_vAddIDs.push_back(sted.nEventID);
				Json::Value jsMsg;
				jsMsg["clubID"] = getClub()->getClubID();
				jsMsg["uid"] = nMemberID;
				jsMsg["targetUID"] = nFireUID;
				getClub()->getClubMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, nFireUID, eAsync_Club_Fire, jsMsg);
				LOGFMTE("User apply fire player in club complete, uid = %u, clubID = %u, fireUID = %u", nMemberID, getClub()->getClubID(), nFireUID);
				Json::Value jsResult;
				jsResult["ret"] = 0;
				jsResult["fireUID"] = nFireUID;
				sendMsgToClient(jsResult, nmsgType, nSenderID);
			}
			else {
				LOGFMTE("User apply fire player in club error, memberData refused, uid = %u, clubID = %u, fireUID = %u", nMemberID, getClub()->getClubID(), nFireUID);
				Json::Value jsResult;
				jsResult["ret"] = 4;
				sendMsgToClient(jsResult, nmsgType, nSenderID);
			}
		}
		else {
			LOGFMTE("User apply fire player in club error, level is not legal, uid = %u, clubID = %u, fireUID = %u", nMemberID, getClub()->getClubID(), nFireUID);
			Json::Value jsResult;
			jsResult["ret"] = 3;
			sendMsgToClient(jsResult, nmsgType, nSenderID);
		}
		return true;
	}

	if (MSG_CLUB_DISMISS_CLUB == nmsgType) {
		uint32_t nMemberID = recvValue["uid"].isUInt() ? recvValue["uid"].asUInt() : 0;
		if (nMemberID == 0) {
			LOGFMTE("User apply dismiss club error, userID is missing, sessionID = %u, clubID = %u", nSenderID, getClub()->getClubID());
			Json::Value jsResult;
			jsResult["ret"] = 1;
			sendMsgToClient(jsResult, nmsgType, nSenderID);
			return true;
		}
		if (getClub()->getClubMemberData()->checkUpdateLevel(nMemberID, getEventLevel(eClubEventType_Dismiss)) == false) {
			LOGFMTE("User apply dismiss club error, user do not have enough level, uid = %u, clubID = %u", nMemberID, getClub()->getClubID());
			Json::Value jsResult;
			jsResult["ret"] = 2;
			sendMsgToClient(jsResult, nmsgType, nSenderID);
			return true;
		}

		if (getClub()->dismissClub()) {
			stEventData sted;
			sted.nEventID = getClub()->getClubMgr()->generateEventID();
			sted.nDisposerUID = nMemberID;
			sted.nState = eClubEventState_Accede;
			sted.nPostTime = time(NULL);
			sted.nEventType = eClubEventType_Dismiss;
			sted.nLevel = getEventLevel(eClubEventType_Dismiss);
			sted.jsDetail = recvValue;
			m_mAllEvents[sted.nEventID] = sted;
			m_vAddIDs.push_back(sted.nEventID);
			Json::Value jsMsg;
			jsMsg["clubID"] = getClub()->getClubID();
			jsMsg["uid"] = nMemberID;
			getClub()->getClubMemberData()->pushAsyncRequestToAll(ID_MSG_PORT_DATA, eAsync_Club_Dismiss, jsMsg);
			Json::Value jsResult;
			jsResult["ret"] = 0;
			jsResult["clubID"] = getClub()->getClubID();
			sendMsgToClient(jsResult, nmsgType, nSenderID);
		}
		else {
			Json::Value jsResult;
			jsResult["ret"] = 3;
			sendMsgToClient(jsResult, nmsgType, nSenderID);
		}
		return true;
	}
	if (MSG_CLUB_APPLY_JOIN == nmsgType) {
		uint32_t nMemberID = recvValue["uid"].isUInt() ? recvValue["uid"].asUInt() : 0;
		Json::Value jsResult;
		if (nMemberID == 0) {
			LOGFMTE("User apply join to club error, userID is missing, sessionID = %u, clubID = %u", nSenderID, getClub()->getClubID());
			jsResult["ret"] = 1;
			sendMsgToClient(jsResult, nmsgType, nSenderID);
			return true;
		}
		if (getClub()->getClubMemberData()->isNotJoin(nMemberID) == false) {
			LOGFMTE("User apply join to club error, is already join, uid = %u, clubID = %u", nMemberID, getClub()->getClubID());
			jsResult["ret"] = 2;
			sendMsgToClient(jsResult, nmsgType, nSenderID);
			return true;
		}
		if (hasApplayJoin(nMemberID)) {
			LOGFMTE("User apply join to club error, is already apply join, uid = %u, clubID = %u", nMemberID, getClub()->getClubID());
			jsResult["ret"] = 3;
			sendMsgToClient(jsResult, nmsgType, nSenderID);
			return true;
		}
		stEventData sted;
		sted.nEventID = getClub()->getClubMgr()->generateEventID();
		sted.nDisposerUID = 0;
		sted.nState = eClubEventState_Wait;
		sted.nPostTime = time(NULL);
		sted.nEventType = eClubEventType_AppcationJoin;
		sted.nLevel = getEventLevel(eClubEventType_AppcationJoin);
		sted.jsDetail = recvValue;
		m_mAllEvents[sted.nEventID] = sted;
		m_vAddIDs.push_back(sted.nEventID);

		LOGFMTE("User apply join to club, is already join, sessionID = %u, clubID = %u", nSenderID, getClub()->getClubID());
		jsResult["ret"] = 0;
		sendMsgToClient(jsResult, nmsgType, nSenderID);

		Json::Value jsMsg;
		jsMsg["clubID"] = getClub()->getClubID();
		jsMsg["type"] = eClubEventType_AppcationJoin;
		getClub()->getClubMemberData()->pushAsyncRequestToLevelNeed(ID_MSG_PORT_DATA, eAsync_player_club_Push_Event, jsMsg, sted.nLevel);
		return true;
	}

	if (MSG_CLUB_EVENT_JOIN == nmsgType) {
		Json::Value jsMsg, jsEvents;
		uint8_t i = 0;
		for (auto& ref : m_mAllEvents) {
			auto& data = ref.second;
			if (data.nEventType == eClubEventType_AppcationJoin) {
				Json::Value jsEvent;
				jsEvent["eventID"] = data.nEventID;
				jsEvent["time"] = data.nPostTime;
				jsEvent["state"] = data.nState;
				jsEvent["disposer"] = data.nDisposerUID;
				jsEvent["detail"] = data.jsDetail;
				jsEvents[jsEvents.size()] = jsEvent;
				if (++i >= ENTRY_RECORDER_LIST_LIMIT) {
					break;
				}
			}
		}
		jsMsg["ret"] = 0;
		jsMsg["events"] = jsEvents;
		sendMsgToClient(jsMsg, nmsgType, nSenderID);
		return true;
	}

	if (MSG_CLUB_EVENT_ENTRY == nmsgType) {
		Json::Value jsMsg, jsEvents;
		bool isMTT = recvValue["mtt"].isUInt() ? recvValue["mtt"].asBool() : false;
		for (auto& ref : m_mAllEvents) {
			auto& data = ref.second;
			if (data.nEventType == eClubEventType_AppcationEntry && data.nState == eClubEventState_Wait) {
				if (data.jsDetail["mtt"].asBool() == isMTT) {
					Json::Value jsEvent;
					jsEvent["eventID"] = data.nEventID;
					jsEvent["time"] = data.nPostTime;
					jsEvent["detail"] = data.jsDetail;
					jsEvents[jsEvents.size()] = jsEvent;
				}
			}
		}
		jsMsg["ret"] = 0;
		jsMsg["events"] = jsEvents;
		sendMsgToClient(jsMsg, nmsgType, nSenderID);
		return true;
	}

	if (MSG_CLUB_EVENT_ENTRY_RECORDER == nmsgType) {
		Json::Value jsMsg, jsEvents;
		bool isMTT = recvValue["mtt"].isUInt() ? recvValue["mtt"].asBool() : false;
		uint8_t i = 0;
		for (auto& ref : m_mAllEvents) {
			auto& data = ref.second;
			if (data.nEventType == eClubEventType_AppcationEntry && data.nState != eClubEventState_Wait) {
				if (data.jsDetail["mtt"].asBool() == isMTT) {
					Json::Value jsEvent;
					jsEvent["eventID"] = data.nEventID;
					jsEvent["time"] = data.nPostTime;
					jsEvent["state"] = data.nState;
					jsEvent["disposer"] = data.nDisposerUID;
					jsEvent["detail"] = data.jsDetail;
					jsEvents[jsEvents.size()] = jsEvent;
					if (++i >= ENTRY_RECORDER_LIST_LIMIT) {
						break;
					}
				}
			}
		}
		jsMsg["ret"] = 0;
		jsMsg["events"] = jsEvents;
		sendMsgToClient(jsMsg, nmsgType, nSenderID);
		return true;
	}

	if (MSG_CLUB_EVENT_APPLY_TREAT == nmsgType) {
		uint32_t nPlayerID = recvValue["uid"].isUInt() ? recvValue["uid"].asUInt() : 0;
		uint32_t nEventID = recvValue["eventID"].isUInt() ? recvValue["eventID"].asUInt() : 0;
		Json::Value jsResult;
		jsResult["eventID"] = nEventID;
		if (nPlayerID == 0) {
			LOGFMTE("User apply treat club event error, userID is missing, sessionID = %u, clubID = %u", nSenderID, getClub()->getClubID());
			jsResult["ret"] = 1;
			sendMsgToClient(jsResult, nmsgType, nSenderID);
			return true;
		}

		if (nEventID == 0) {
			LOGFMTE("User apply treat club event error, eventID is missing, playerID = %u, clubID = %u", nPlayerID, getClub()->getClubID());
			jsResult["ret"] = 2;
			sendMsgToClient(jsResult, nmsgType, nSenderID);
			return true;
		}

		if (recvValue["state"].isNull() || recvValue["state"].isUInt() == false) {
			LOGFMTE("User apply treat club event error, state is missing, playerID = %u, clubID = %u", nPlayerID, getClub()->getClubID());
			jsResult["ret"] = 3;
			sendMsgToClient(jsResult, nmsgType, nSenderID);
			return true;
		}
		uint8_t nState = recvValue["state"].asUInt();

		treatEvent(nEventID, nPlayerID, nState, nSenderID, recvValue);
		return true;
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

	if (MSG_CLUB_EVENT_GRANT_FOUNDATION == nmsgType) {
		Json::Value jsMsg;
		uint32_t nUserID = recvValue["uid"].isUInt() ? recvValue["uid"].asUInt() : 0;
		if (nUserID == 0) {
			jsMsg["ret"] = 1;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("grant foundation error, userID is miss, clubID = %u", getClub()->getClubID());
			return true;
		}
		if (getClub()->getClubMemberData()->checkUpdateLevel(nUserID, eClubUpdateLevel_GrantFoundation) == false) {
			jsMsg["ret"] = 2;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("grant foundation error, user level is not enough, clubID = %u, userID = %u", getClub()->getClubID(), nUserID);
			return true;
		}
		uint32_t nMemberUID = recvValue["memberUID"].isUInt() ? recvValue["memberUID"].asUInt() : 0;
		if (nMemberUID == 0) {
			jsMsg["ret"] = 3;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("grant foundation error, memberUID is miss, clubID = %u, userID = %u", getClub()->getClubID(), nUserID);
			return true;
		}
		if (getClub()->getClubMemberData()->isNotJoin(nMemberUID)) {
			jsMsg["ret"] = 4;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("grant foundation error, memberUID is not join, clubID = %u, userID = %u, memberUID = %u", getClub()->getClubID(), nUserID, nMemberUID);
			return true;
		}
		uint32_t nAmount = recvValue["amount"].isUInt() ? recvValue["amount"].asUInt() : 0;
		if (nAmount == 0) {
			jsMsg["ret"] = 5;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("grant foundation error, amount is miss, clubID = %u, userID = %u, memberUID = %u", getClub()->getClubID(), nUserID, nMemberUID);
			return true;
		}
		if (getClub()->getClubMemberData()->grantFoundation(nUserID, nMemberUID, nAmount) == false) {
			jsMsg["ret"] = 6;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("grant foundation error, foundation is not enough? clubID = %u, userID = %u, memberUID = %u, nAmount = %u", getClub()->getClubID(), nUserID, nMemberUID, nAmount);
			return true;
		}
		jsMsg["ret"] = 0;
		jsMsg["amount"] = nAmount;
		sendMsgToClient(jsMsg, nmsgType, nSenderID);
		LOGFMTI("grant foundation successful with info: clubID = %u, userID = %u, memberUID = %u, nAmount = %u", getClub()->getClubID(), nUserID, nMemberUID, nAmount);
		
		//²åÈëÊÂ¼þ
		stEventData sted;
		sted.nEventID = getClub()->getClubMgr()->generateEventID();
		sted.nDisposerUID = nUserID;
		sted.nState = eClubEventState_Accede;
		sted.nPostTime = time(NULL);
		sted.nEventType = eClubEventType_GrantFoundation;
		sted.nLevel = eClubUpdateLevel_GrantFoundation;
		sted.jsDetail = recvValue;
		m_mAllEvents[sted.nEventID] = sted;
		m_vAddIDs.push_back(sted.nEventID);
		return true;
	}

	return false;
}

bool CClubEvent::onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult) {
	if (eAsync_Club_AddFoundation == nRequestType) {
		int32_t nAmount = jsReqContent["amount"].asInt();
		getClub()->addFoundation(nAmount);
		jsResult["ret"] = 0;
		stEventData sted;
		sted.nEventID = getClub()->getClubMgr()->generateEventID();
		sted.nDisposerUID = 0;
		sted.nState = eClubEventState_Accede;
		sted.nPostTime = time(NULL);
		sted.nEventType = eClubEventType_PlayerAddFoundation;
		sted.nLevel = getEventLevel(eClubEventType_PlayerAddFoundation);
		sted.jsDetail = jsReqContent;
		m_mAllEvents[sted.nEventID] = sted;
		m_vAddIDs.push_back(sted.nEventID);
		return true;
	}

	if (eAsync_League_AddIntegration == nRequestType) {
		auto amount = jsReqContent["amount"].asUInt();
		if (amount) {
			getClub()->addIntegration((int32_t)amount);
		}
		stEventData sted;
		sted.nEventID = getClub()->getClubMgr()->generateEventID();
		sted.nDisposerUID = 0;
		sted.nState = eClubEventState_Accede;
		sted.nPostTime = time(NULL);
		sted.nEventType = eClubEventType_ReciveIntegration;
		sted.nLevel = getEventLevel(eClubEventType_ReciveIntegration);
		sted.jsDetail = jsReqContent;
		m_mAllEvents[sted.nEventID] = sted;
		m_vAddIDs.push_back(sted.nEventID);
		return true;
	}

	if (eAsync_club_apply_DragIn == nRequestType) {
		auto nAmount = jsReqContent["amount"].asUInt();
		auto roomID = jsReqContent["roomID"].asUInt();
		auto nUserID = jsReqContent["uid"].asUInt();
		//auto nLeagueID = jsReqContent["leagueID"].asUInt();
		for (auto& ref : m_mAllEvents) {
			if (ref.second.nEventType == eClubEventType_AppcationEntry && ref.second.nState == eClubEventState_Wait) {
				if (ref.second.jsDetail["uid"].asUInt() == nUserID && ref.second.jsDetail["roomID"].asUInt() == roomID) {
					jsResult["ret"] = 1;
					return true;
				}
			}
		}
		if (getClub()->getClubMemberData()->isNotJoin(nUserID)) {
			jsResult["ret"] = 2;
			return true;
		}
		stEventData sted;
		sted.nEventID = getClub()->getClubMgr()->generateEventID();
		sted.nDisposerUID = 0;
		sted.nState = eClubEventState_Wait;
		sted.nPostTime = time(NULL);
		sted.nEventType = eClubEventType_AppcationEntry;
		sted.nLevel = getEventLevel(eClubEventType_AppcationEntry);
		Json::Value jsDetail;
		jsDetail = jsReqContent;
		auto cRemark = getClub()->getClubMemberData()->getMemberRemark(nUserID);
		jsDetail["remark"] = cRemark == nullptr ? "" : cRemark;
		sted.jsDetail = jsDetail;
		m_mAllEvents[sted.nEventID] = sted;
		m_vAddIDs.push_back(sted.nEventID);
		jsResult["ret"] = 0;
		Json::Value jsMsg;
		jsMsg["clubID"] = getClub()->getClubID();
		jsMsg["type"] = eClubEventType_AppcationEntry;
		getClub()->getClubMemberData()->pushAsyncRequestToLevelNeed(ID_MSG_PORT_DATA, eAsync_player_club_Push_Event, jsMsg, sted.nLevel);
		
		bool needVerify = jsReqContent["needVerify"].isUInt() ? jsReqContent["needVerify"].asBool() : true;
		if (needVerify == false) {
			autoTreatEvent(sted.nEventID, eClubEventState_Accede);
		}
		return true;
	}

	return false;
}

void CClubEvent::timerSave() {
	if (m_bReadingDB) {
		return;
	}

	if (m_vDirtyIDs.empty() && m_vAddIDs.empty()) {
		return;
	}

	for (auto nAddUID : m_vAddIDs) {
		if (m_mAllEvents.find(nAddUID) == m_mAllEvents.end()) {
			LOGFMTE("caution: add club event error, can not find event info, clubID = %u , eventID = %u, caution!!!!!", getClub()->getClubID(), nAddUID);
			continue;
		}
		auto info = m_mAllEvents[nAddUID];

		Json::StyledWriter ss;
		auto jsDetail = ss.write(info.jsDetail);
		Json::Value jssql;
		char pBuffer[512] = { 0 };
		if (info.nDisposerUID) {
			sprintf_s(pBuffer, "insert into clubevent (eventID, clubID, postTime, eventType, level, state, disposerUID, detail) values (%u, %u, from_unixtime( %u ), %u, %u, %u, %u,", info.nEventID, getClub()->getClubID(), info.nPostTime, info.nEventType, info.nLevel, info.nState, info.nDisposerUID);
		}
		else {
			sprintf_s(pBuffer, "insert into clubevent (eventID, clubID, postTime, eventType, level, state, detail) values (%u, %u, from_unixtime( %u ), %u, %u, %u,", info.nEventID, getClub()->getClubID(), info.nPostTime, info.nEventType, info.nLevel, info.nState);
		}
		//sprintf_s(pBuffer, "insert into clubevent (eventID, clubID, postTime, eventType, level, state, detail) values (%u, %u, %u, %u, %u, %u,", info.nEventID, getClub()->getClubID(), info.nPostTime, info.nEventType, info.nLevel, info.nState);
		std::ostringstream ssSql;
		ssSql << pBuffer << " ' " << jsDetail << " ' );";
		jssql["sql"] = ssSql.str();
		auto pReqQueue = m_pClub->getClubMgr()->getSvrApp()->getAsynReqQueue();
		pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, getClub()->getClubID(), eAsync_DB_Add, jssql);
	}
	m_vAddIDs.clear();

	for (auto nDirtyUID : m_vDirtyIDs) {
		if (m_mAllEvents.find(nDirtyUID) == m_mAllEvents.end()) {
			LOGFMTE("caution: save club event error, can not find event info, clubID = %u , eventID = %u, caution!!!!!", getClub()->getClubID(), nDirtyUID);
			continue;
		}
		auto info = m_mAllEvents[nDirtyUID];
		Json::Value jssql;
		char pBuffer[512] = { 0 };
		if (info.nDisposerUID) {
			sprintf_s(pBuffer, "update clubevent set state = %u, disposerUID = %u where eventID = %u;", info.nState, info.nDisposerUID, nDirtyUID);
		}
		else {
			sprintf_s(pBuffer, "update clubevent set state = %u where eventID = %u;", info.nState, nDirtyUID);
		}
		
		std::string str = pBuffer;
		jssql["sql"] = pBuffer;
		auto pReqQueue = m_pClub->getClubMgr()->getSvrApp()->getAsynReqQueue();
		pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, getClub()->getClubID(), eAsync_DB_Update, jssql);
	}
	m_vDirtyIDs.clear();

	for (auto it = m_mAllEvents.begin(); it != m_mAllEvents.end();) {
		if (it->second.nPostTime + AUTO_TREET_EVENT_TIME < time(NULL) && it->second.nState == eClubEventState_Wait) {
			it->second.nState = eClubEventState_Decline;
			m_vDirtyIDs.push_back(it->first);
		}

		if (it->second.nPostTime + AUTO_RELEASE_EVENT_TIME < time(NULL) && eventIsDirty(it->first) == false) {
			m_mAllEvents.erase(it++);
		}
		else {
			it++;
		}
	}
}

void CClubEvent::readEventFormDB(uint32_t nOffset) {
	m_bReadingDB = true;
	std::ostringstream ss;
	ss << "SELECT eventID, unix_timestamp(postTime) as postTime, eventType, level, state, disposerUID, detail FROM clubevent where clubID = " << (UINT)m_pClub->getClubID() << " order by eventID desc limit 10 offset " << (UINT)nOffset << ";";
	Json::Value jsReq;
	jsReq["sql"] = ss.str();
	m_pClub->getClubMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB, rand(), eAsync_DB_Select, jsReq, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut) {
		if (isTimeOut)
		{
			LOGFMTE("caution: load club event from DB time out, clubID = %u , caution!!!!!", getClub()->getClubID());
			doProcessAfterReadDB();
			return;
		}

		//LOGFMTE("caution: success load club event from DB, clubID = %u , caution!!!!!", getClub()->getClubID());
		uint32_t nAft = retContent["afctRow"].asUInt();
		auto jsData = retContent["data"];
		if (nAft == 0 || jsData.isNull())
		{
			doProcessAfterReadDB();
			return;
		}

		//LOGFMTE("caution: success load club event from DB, clubID = %u with %u recorder caution!!!!!", getClub()->getClubID(), nAft);

		for (uint32_t nRowIdx = 0; nRowIdx < nAft; ++nRowIdx)
		{
			auto jsRow = jsData[nRowIdx];
			auto nEventID = jsRow["eventID"].asUInt();
			auto iter = m_mAllEvents.find(nEventID);
			if (iter != m_mAllEvents.end())
			{
				LOGFMTE("bug: why already have this club event eventID = %u , in club id = %u double read , bug!!!!!!", nEventID, getClub()->getClubID());
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
		if (nSize > 599) {
			doProcessAfterReadDB();
			return;
		}
		readEventFormDB(nSize);
	});
}

void CClubEvent::doProcessAfterReadDB() {
	m_bReadingDB = false;
}

uint8_t CClubEvent::getEventLevel(uint8_t nEventType) {
	switch (nEventType)
	{
	case eClubEventType_AppcationJoin :
	{
		return eClubMemberLevel_Admin;
	}
	case eClubEventType_AppcationEntry :
	{
		return eClubMemberLevel_Admin;
	}
	case eClubEventType_Dismiss :
	{
		return eClubMemberLevel_Creator;
	}
	case eClubEventType_FirePlayer :
	{
		return eClubMemberLevel_Admin;
	}
	case eClubEventType_PlayerAddFoundation :
	{
		return eClubMemberLevel_Admin;
	}
	default :
	{
		return eClubMemberLevel_None;
	}
	}
}

bool CClubEvent::eventIsDirty(uint32_t nEventID) {
	return std::find(m_vDirtyIDs.begin(), m_vDirtyIDs.end(), nEventID) == m_vDirtyIDs.end() &&
		std::find(m_vAddIDs.begin(), m_vAddIDs.end(), nEventID) == m_vAddIDs.end();
}

void CClubEvent::autoTreatEvent(uint32_t nEventID, uint8_t nState) {
	auto& itEvent = m_mAllEvents.find(nEventID);
	if (itEvent == m_mAllEvents.end()) {
		return;
	}

	if (itEvent->second.bWaiting) {
		return;
	}

	if (itEvent->second.nState != eClubEventState_Wait) {
		return;
	}

	switch (itEvent->second.nEventType) {
	case eClubEventType_AppcationEntry: {
		if (nState == eClubEventState_Accede) {
			auto nMemberUID = itEvent->second.jsDetail["uid"].asUInt();
			if (getClub()->getClubMemberData()->isNotJoin(nMemberUID)) {
				itEvent->second.nState = eClubEventState_Decline;
				itEvent->second.nDisposerUID = 0;
				m_vDirtyIDs.push_back(itEvent->first);
				return;
			}

			auto nRoomID = itEvent->second.jsDetail["roomID"].asUInt();
			auto nAmount = itEvent->second.jsDetail["amount"].asUInt();
			auto nLeagueID = itEvent->second.jsDetail["leagueID"].asUInt();
			bool bMTT = itEvent->second.jsDetail["mtt"].isUInt() ? itEvent->second.jsDetail["mtt"].asBool() : false;

			Json::Value jsMsg;
			jsMsg["amount"] = itEvent->second.jsDetail["amount"];
			jsMsg["roomID"] = nRoomID;
			jsMsg["uid"] = nMemberUID;
			jsMsg["clubID"] = getClub()->getClubID();
			jsMsg["eventID"] = itEvent->first;
			if (bMTT) {
				jsMsg["mtt"] = 1;
				jsMsg["initialCoin"] = itEvent->second.jsDetail["initialCoin"];
			}
			auto sendPort = itEvent->second.jsDetail["port"].asUInt();
			auto sendTargetID = nRoomID;
			if (nLeagueID && bMTT == false) {
				jsMsg["leagueID"] = nLeagueID;
				jsMsg["port"] = itEvent->second.jsDetail["port"].asUInt();
				sendPort = ID_MSG_PORT_DATA;
				sendTargetID = nLeagueID;
			}
			itEvent->second.bWaiting = true;
			getClub()->getClubMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(sendPort, sendTargetID, eAsync_club_agree_DragIn, jsMsg, [this, nRoomID, nAmount, itEvent](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut) {
				itEvent->second.bWaiting = false;
				if (isTimeOut || retContent["ret"].asUInt() || itEvent->second.nState != eClubEventState_Wait) {
					itEvent->second.nState = eClubEventState_Decline;
					itEvent->second.nDisposerUID = 0;
					m_vDirtyIDs.push_back(itEvent->first);
					return;
				}

				itEvent->second.nState = eClubEventState_Accede;
				itEvent->second.nDisposerUID = 0;
				m_vDirtyIDs.push_back(itEvent->first);
				Json::Value jsResult;
				jsResult["eventID"] = itEvent->first;
				jsResult["ret"] = 0;
				getClub()->getClubMemberData()->pushAsyncRequestToLevelNeed(ID_MSG_PORT_DATA, eAsync_club_Treat_Event_Message, jsResult, eClubMemberLevel_Admin);
			}, itEvent->first);
		}
		else {
			itEvent->second.nState = eClubEventState_Decline;
			itEvent->second.nDisposerUID = 0;
			m_vDirtyIDs.push_back(itEvent->first);
		}
		break;
	}
	}
}

uint8_t CClubEvent::treatEvent(uint32_t nEventID, uint32_t nPlayerID, uint8_t nState, uint32_t nSenderID, Json::Value& recvValue) {
	uint8_t nRet = 0;
	auto& itEvent = m_mAllEvents.find(nEventID);
	if (itEvent == m_mAllEvents.end()) {
		LOGFMTE("User apply treat club event error, can not find event, eventID = %u, playerID = %u, clubID = %u", nEventID, nPlayerID, getClub()->getClubID());
		Json::Value jsResult;
		jsResult["eventID"] = nEventID;
		jsResult["ret"] = 5;
		sendMsgToClient(jsResult, MSG_CLUB_EVENT_APPLY_TREAT, nSenderID);
		return 5;
	}

	if (itEvent->second.bWaiting) {
		LOGFMTE("User apply treat club event error, event is already in treating, eventID = %u, playerID = %u, clubID = %u", nEventID, nPlayerID, getClub()->getClubID());
		Json::Value jsResult;
		jsResult["eventID"] = nEventID;
		jsResult["ret"] = 6;
		sendMsgToClient(jsResult, MSG_CLUB_EVENT_APPLY_TREAT, nSenderID);
		return 6;
	}

	if (itEvent->second.nState != eClubEventState_Wait) {
		LOGFMTE("User apply treat club event error, event is already be treated, eventID = %u, playerID = %u, clubID = %u", nEventID, nPlayerID, getClub()->getClubID());
		Json::Value jsResult;
		jsResult["eventID"] = nEventID;
		jsResult["ret"] = 6;
		sendMsgToClient(jsResult, MSG_CLUB_EVENT_APPLY_TREAT, nSenderID);
		return 6;
	}

	if (getClub()->getClubMemberData()->checkUpdateLevel(nPlayerID, itEvent->second.nLevel) == false) {
		LOGFMTE("User apply treat club event error, player level is not enough, eventID = %u, playerID = %u, clubID = %u", nEventID, nPlayerID, getClub()->getClubID());
		Json::Value jsResult;
		jsResult["eventID"] = nEventID;
		jsResult["ret"] = 7;
		sendMsgToClient(jsResult, MSG_CLUB_EVENT_APPLY_TREAT, nSenderID);
		return 7;
	}

	bool isWait = false;

	if (nState == eClubEventState_Accede) {
		switch (itEvent->second.nEventType) {
		case eClubEventType_AppcationJoin: {
			auto cName = recvValue["name"].asCString();
			uint32_t nMemberUID = itEvent->second.jsDetail["uid"].asUInt();
			if (getClub()->getClubMemberData()->addMember(nMemberUID, cName) == false) {
				LOGFMTE("User apply treat club event error, club refused could be full? eventID = %u, playerID = %u, clubID = %u", nEventID, nPlayerID, getClub()->getClubID());
				Json::Value jsResult;
				jsResult["eventID"] = nEventID;
				jsResult["ret"] = 8;
				sendMsgToClient(jsResult, MSG_CLUB_EVENT_APPLY_TREAT, nSenderID);
				return 8;
			}
			Json::Value jsMsg;
			jsMsg["targetUID"] = nMemberUID;
			jsMsg["clubID"] = getClub()->getClubID();
			jsMsg["agentID"] = nPlayerID;
			getClub()->getClubMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, nMemberUID, eAsync_Club_Join, jsMsg);
			itEvent->second.nState = eClubEventState_Accede;
			break;
		}
		case eClubEventType_AppcationEntry: {
			isWait = true;
			auto nMemberUID = itEvent->second.jsDetail["uid"].asUInt();
			if (getClub()->getClubMemberData()->isNotJoin(nMemberUID)) {
				LOGFMTE("User apply treat entry event error, uid is not join? eventID = %u, playerID = %u, clubID = %u", nEventID, nPlayerID, getClub()->getClubID());
				itEvent->second.nState = eClubEventState_Decline;
				itEvent->second.nDisposerUID = 0;
				m_vDirtyIDs.push_back(itEvent->first);
				Json::Value jsResult;
				jsResult["eventID"] = nEventID;
				jsResult["ret"] = 10;
				sendMsgToClient(jsResult, MSG_CLUB_EVENT_APPLY_TREAT, nSenderID);
				return 10;
			}
			auto nRoomID = itEvent->second.jsDetail["roomID"].asUInt();
			auto nAmount = itEvent->second.jsDetail["amount"].asUInt();
			auto nLeagueID = itEvent->second.jsDetail["leagueID"].asUInt();
			bool bMTT = itEvent->second.jsDetail["mtt"].isUInt() ? itEvent->second.jsDetail["mtt"].asBool() : false;
			//if (getClub()->getClubGameData()->isClubCreateThisRoom(nRoomID) == false) {
				/*if (getClub()->getIntegration() < nAmount) {
					Json::Value jsResult;
					jsResult["ret"] = 11;
					sendMsgToClient(jsResult, MSG_CLUB_EVENT_APPLY_TREAT, nSenderID);
					return 11;
				}*/
				/*else {
					getClub()->addIntegration(-1 * (int32_t)nAmount);
				}*/
			//}
			Json::Value jsMsg;
			jsMsg["amount"] = itEvent->second.jsDetail["amount"];
			jsMsg["roomID"] = nRoomID;
			jsMsg["uid"] = nMemberUID;
			jsMsg["clubID"] = getClub()->getClubID();
			jsMsg["eventID"] = itEvent->first;
			if (bMTT) {
				jsMsg["mtt"] = 1;
				jsMsg["initialCoin"] = itEvent->second.jsDetail["initialCoin"];
			}
			auto sendPort = itEvent->second.jsDetail["port"].asUInt();
			auto sendTargetID = nRoomID;
			//getClub()->getClubMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(itEvent->second.jsDetail["port"].asUInt(), nRoomID, eAsync_club_agree_DragIn, jsMsg);
			if (nLeagueID && bMTT == false) {
				jsMsg["leagueID"] = nLeagueID;
				jsMsg["port"] = itEvent->second.jsDetail["port"].asUInt();
				sendPort = ID_MSG_PORT_DATA;
				sendTargetID = nLeagueID;
			}
			itEvent->second.bWaiting = true;
			getClub()->getClubMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(sendPort, sendTargetID, eAsync_club_agree_DragIn, jsMsg, [this, nRoomID, nSenderID, nAmount, itEvent, nPlayerID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut) {
				itEvent->second.bWaiting = false;
				if (isTimeOut) {
					Json::Value jsResult;
					jsResult["eventID"] = itEvent->first;
					jsResult["ret"] = 12;
					sendMsgToClient(jsResult, MSG_CLUB_EVENT_APPLY_TREAT, nSenderID);
					return;
				}

				if (retContent["ret"].asUInt()) {
					Json::Value jsResult;
					jsResult["eventID"] = itEvent->first;
					jsResult["ret"] = retContent["ret"].asUInt() == 11 ? 11 : 13;
					sendMsgToClient(jsResult, MSG_CLUB_EVENT_APPLY_TREAT, nSenderID);
					return;
				}

				if (itEvent->second.nState != eClubEventState_Wait) {
					LOGFMTE("User apply treat club event error, event is already be treated, eventID = %u, playerID = %u, clubID = %u", itEvent->second.nEventID, nPlayerID, getClub()->getClubID());
					Json::Value jsResult;
					jsResult["eventID"] = itEvent->second.nEventID;
					jsResult["ret"] = 6;
					sendMsgToClient(jsResult, MSG_CLUB_EVENT_APPLY_TREAT, nSenderID);
					return;
				}

				itEvent->second.nState = eClubEventState_Accede;
				itEvent->second.nDisposerUID = nPlayerID;
				m_vDirtyIDs.push_back(itEvent->first);
				/*if (getClub()->getClubGameData()->isClubCreateThisRoom(nRoomID) == false) {
				getClub()->addIntegration(-1 * (int32_t)nAmount);
				}*/
				Json::Value jsResult;
				jsResult["eventID"] = itEvent->first;
				jsResult["ret"] = 0;
				sendMsgToClient(jsResult, MSG_CLUB_EVENT_APPLY_TREAT, nSenderID);
				getClub()->getClubMemberData()->pushAsyncRequestToLevelNeed(ID_MSG_PORT_DATA, eAsync_club_Treat_Event_Message, jsResult, eClubMemberLevel_Admin);
			}, itEvent->first);
			break;
		}
		default: {
			LOGFMTE("User apply treat club event error, event type can not be treated, eventID = %u, playerID = %u, clubID = %u", nEventID, nPlayerID, getClub()->getClubID());
			Json::Value jsResult;
			jsResult["eventID"] = nEventID;
			jsResult["ret"] = 9;
			sendMsgToClient(jsResult, MSG_CLUB_EVENT_APPLY_TREAT, nSenderID);
			return 9;
		}
		}
	}
	else {
		itEvent->second.nState = eClubEventState_Decline;
		if (itEvent->second.nEventType == eClubEventType_AppcationEntry) {
			Json::Value jsMsg, jsMsgP;
			auto nMemberUID = itEvent->second.jsDetail["uid"].asUInt();
			auto nRoomID = itEvent->second.jsDetail["roomID"].asUInt();
			jsMsg["roomID"] = nRoomID;
			jsMsg["uid"] = nMemberUID;
			jsMsg["clubID"] = getClub()->getClubID();
			jsMsg["eventID"] = itEvent->first;
			auto reqQueue = getClub()->getClubMgr()->getSvrApp()->getAsynReqQueue();
			reqQueue->pushAsyncRequest(itEvent->second.jsDetail["port"].asUInt(), nRoomID, eAsync_club_decline_DragIn, jsMsg);
			jsMsgP["targetUID"] = nMemberUID;
			jsMsgP["roomID"] = nRoomID;
			jsMsgP["clubID"] = getClub()->getClubID();
			jsMsgP["uid"] = nPlayerID;
			bool isMTT = itEvent->second.jsDetail["mtt"].asBool();
			if (isMTT) {
				jsMsgP["dragIn"] = itEvent->second.jsDetail["dragIn"];
			}
			reqQueue->pushAsyncRequest(ID_MSG_PORT_DATA, nMemberUID, eAsync_player_club_decline_DragIn, jsMsgP);
		}
	}
	
	if (isWait) {
		return 0;
	}
	itEvent->second.nDisposerUID = nPlayerID;
	m_vDirtyIDs.push_back(itEvent->first);
	Json::Value jsResult;
	jsResult["eventID"] = nEventID;
	jsResult["ret"] = 0;
	jsResult["state"] = nState;
	sendMsgToClient(jsResult, MSG_CLUB_EVENT_APPLY_TREAT, nSenderID);
	getClub()->getClubMemberData()->pushAsyncRequestToLevelNeed(ID_MSG_PORT_DATA, eAsync_club_Treat_Event_Message, jsResult, eClubMemberLevel_Admin);
	return 0;
}

bool CClubEvent::hasApplayJoin(uint32_t nUserID) {
	for (auto ref : m_mAllEvents) {
		if (ref.second.nEventType == eClubEventType_AppcationJoin && ref.second.nState == eClubEventState_Wait) {
			if (ref.second.jsDetail["uid"].asUInt() == nUserID) {
				return true;
			}
		}
	}
	return false;
}