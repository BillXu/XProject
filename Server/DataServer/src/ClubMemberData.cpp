#include "ClubMemberData.h"
#include "Club.h"
#include "ClubManager.h"
#include "ISeverApp.h"
#include "AsyncRequestQuene.h"
#include "log4z.h"
#include <time.h>
#include <algorithm>
CClubMemberData::CClubMemberData()
{
	m_eType = eClubComponent_MemberData;
	m_bReadingDB = false;
	m_vMemberDertyIDs.clear();
	m_vMemberAddIDs.clear();
	m_mMembers.clear();
}

CClubMemberData::~CClubMemberData() {
	
}

void CClubMemberData::init(CClub* pClub) {
	IClubComponent::init(pClub);
	readMemberFormDB();
}

void CClubMemberData::reset() {
	m_bReadingDB = false;
	m_vMemberDertyIDs.clear();
	m_vMemberAddIDs.clear();
	m_mMembers.clear();
}

bool CClubMemberData::onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort, uint32_t nSenderID) {
	if (MSG_CLUB_MEMBER_DETAIL == nmsgType) {
		Json::Value jsMsg;
		uint32_t nMemberUID = recvValue["memberUID"].isUInt() ? recvValue["memberUID"].asUInt() : 0;
		if (nMemberUID == 0) {
			jsMsg["ret"] = 1;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("request club member detail error, userID is miss, clubID = %u", getClub()->getClubID());
			return true;
		}
		if (isNotJoin(nMemberUID)) {
			jsMsg["ret"] = 4;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("request club member detail error, memberUID is not join, clubID = %u, memberUID = %u", getClub()->getClubID(), nMemberUID);
			return true;
		}
		jsMsg["ret"] = 0;
		jsMsg["clubID"] = getClub()->getClubID();
		jsMsg["uid"] = nMemberUID;
		jsMsg["level"] = getMemberLevel(nMemberUID);
		jsMsg["remark"] = m_mMembers[nMemberUID].cRemark;
		sendMsgToClient(jsMsg, nmsgType, nSenderID);
		return true;
	}

	if (MSG_CLUB_MEMBER_INFO == nmsgType) {
		Json::Value jsMsg, jsUIDs;
		std::vector<stMemberBaseData*> vMembers;
		for (auto& ref : m_mMembers) {
			if (ref.second.nState != eClubState_Delete) {
				vMembers.push_back(&(ref.second));
				/*Json::Value jsMember;
				jsMember["id"] = ref.first;
				jsMember["level"] = ref.second.nLevel;
				jsData[jsData.size()] = jsMember;*/
			}
		}
		std::sort(vMembers.begin(), vMembers.end(), [](stMemberBaseData* st1, stMemberBaseData* st2) {
			if (st1->nLevel != st2->nLevel) {
				return st1->nLevel > st2->nLevel;
			}
			else {
				return st1->nJoinTime < st2->nJoinTime;
			}
		});
		jsMsg["ret"] = 0;
		for (auto& ref : vMembers) {
			jsUIDs[jsUIDs.size()] = ref->nMemberUID;
		}
		jsMsg["clubID"] = getClub()->getClubID();
		jsMsg["members"] = jsUIDs;
		sendMsgToClient(jsMsg, nmsgType, nSenderID);
		return true;
	}

	if (MSG_CLUB_MEMBER_UPDATE_REMARK == nmsgType) {
		Json::Value jsMsg;
		uint32_t nUserID = recvValue["uid"].isUInt() ? recvValue["uid"].asUInt() : 0;
		if (nUserID == 0) {
			jsMsg["ret"] = 1;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club member remark error, userID is miss, clubID = %u", getClub()->getClubID());
			return true;
		}
		if (checkUpdateLevel(nUserID, eClubUpdateLevel_MemberRemark) == false) {
			jsMsg["ret"] = 2;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club member remark error, user level is not enough, clubID = %u, userID = %u", getClub()->getClubID(), nUserID);
			return true;
		}
		uint32_t nMemberUID = recvValue["memberUID"].isUInt() ? recvValue["memberUID"].asUInt() : 0;
		if (nMemberUID == 0) {
			jsMsg["ret"] = 3;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club member remark error, memberUID is miss, clubID = %u, userID = %u", getClub()->getClubID(), nUserID);
			return true;
		}
		if (isNotJoin(nMemberUID)) {
			jsMsg["ret"] = 4;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club member remark error, memberUID is not join, clubID = %u, userID = %u, memberUID = %u", getClub()->getClubID(), nUserID, nMemberUID);
			return true;
		}
		if (getMemberLevel(nUserID) <= getMemberLevel(nMemberUID) && getMemberLevel(nUserID) != eClubMemberLevel_Creator) {
			jsMsg["ret"] = 6;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club member remark error, level is not allowed, clubID = %u, userID = %u, memberUID = %u", getClub()->getClubID(), nUserID, nMemberUID);
			return true;
		}
		m_mMembers[nMemberUID].setRemark(recvValue["remark"].asCString());
		m_vMemberDertyIDs.push_back(nMemberUID);
		jsMsg["ret"] = 0;
		jsMsg["uid"] = nMemberUID;
		sendMsgToClient(jsMsg, nmsgType, nSenderID);
		return true;
	}

	if (MSG_CLUB_INFO_UPDATE_LEVEL == nmsgType) {
		Json::Value jsMsg;
		uint32_t nUserID = recvValue["uid"].isUInt() ? recvValue["uid"].asUInt() : 0;
		if (nUserID == 0) {
			jsMsg["ret"] = 1;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club member level error, userID is miss, clubID = %u", getClub()->getClubID());
			return true;
		}
		if (checkUpdateLevel(nUserID, eClubUpdateLevel_Level) == false) {
			jsMsg["ret"] = 2;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club member level error, user level is not enough, clubID = %u, userID = %u", getClub()->getClubID(), nUserID);
			return true;
		}
		uint32_t nMemberUID = recvValue["memberUID"].isUInt() ? recvValue["memberUID"].asUInt() : 0;
		if (nMemberUID == 0) {
			jsMsg["ret"] = 3;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club member level error, memberUID is miss, clubID = %u, userID = %u", getClub()->getClubID(), nUserID);
			return true;
		}
		if (isNotJoin(nMemberUID)) {
			jsMsg["ret"] = 4;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club member level error, memberUID is not join, clubID = %u, userID = %u, memberUID = %u", getClub()->getClubID(), nUserID, nMemberUID);
			return true;
		}
		if (recvValue["level"].isNull() || recvValue["level"].isUInt() == false) {
			jsMsg["ret"] = 5;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club member level error, level is miss, clubID = %u, userID = %u, memberUID = %u", getClub()->getClubID(), nUserID, nMemberUID);
			return true;
		}
		uint8_t nLevel = recvValue["level"].asUInt();
		if (getMemberLevel(nMemberUID) == nLevel || nLevel >= getMemberLevel(nUserID)) {
			jsMsg["ret"] = 6;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club member level error, level is not allowed, clubID = %u, userID = %u, memberUID = %u, level = %u", getClub()->getClubID(), nUserID, nMemberUID, nLevel);
			return true;
		}
		m_mMembers[nMemberUID].nLevel = nLevel;
		m_vMemberDertyIDs.push_back(nMemberUID);
		jsMsg["ret"] = 0;
		jsMsg["uid"] = nMemberUID;
		sendMsgToClient(jsMsg, nmsgType, nSenderID);
		return true;
	}
	return false;
}

bool CClubMemberData::onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult) {
	if (eAsync_Club_League_T_Player_Check == nRequestType) {
		uint32_t nUserID = jsReqContent["uid"].asUInt();
		uint32_t nMemberUID = jsReqContent["tuid"].asUInt();

		if (isNotJoin(nUserID)) {
			jsResult["ret"] = 1;
			return true;
		}

		if (checkUpdateLevel(nUserID, eClubMemberLevel_Creator) == false || checkUpdateLevel(nUserID, getMemberLevel(nMemberUID), false) == false) {
			jsResult["ret"] = 2;
			return true;
		}

		jsResult["ret"] = 0;
		return true;
	}
	
	if (eAsync_Club_T_Player_Check == nRequestType) {
		uint32_t nUserID = jsReqContent["uid"].asUInt();
		uint32_t nMemberUID = jsReqContent["tuid"].asUInt();
		uint32_t nLeagueID = jsReqContent["leagueID"].asUInt();

		if (isNotJoin(nMemberUID)) {
			jsResult["ret"] = 1;
			return true;
		}

		if (checkUpdateLevel(nUserID, getMemberLevel(nMemberUID), false) == false) {
			if (nLeagueID == 0) {
				jsResult["ret"] = 2;
				return true;
			}
			return false;
		}

		jsResult["ret"] = 0;
		return true;



		/*if (isNotJoin(nUserID) || isNotJoin(nMemberUID)) {
			jsResult["ret"] = 1;
			return true;
		}

		if (checkUpdateLevel(nUserID, getMemberLevel(nMemberUID), false) == false) {
			jsResult["ret"] = 2;
			return true;
		}

		jsResult["ret"] = 0;
		return true;*/
	}

	if (eAsync_club_League_Push_Event == nRequestType) {
		auto nLeagueID = jsReqContent["leagueID"].asUInt();
		auto nType = jsReqContent["type"].asUInt();

		Json::Value jsReq;
		jsReq["clubID"] = getClub()->getClubID();
		jsReq["leagueID"] = nLeagueID;
		jsReq["type"] = nType;
		pushAsyncRequestToLevelNeed(ID_MSG_PORT_DATA, eAsync_player_League_Push_Event, jsReq, eClubMemberLevel_Admin);
		return true;
	}

	if (eAsync_league_ClubQuit_Check == nRequestType) {
		auto nUserID = jsReqContent["uid"].asUInt();
		auto nLeagueID = jsReqContent["leagueID"].asUInt();

		if (checkUpdateLevel(nUserID, eClubUpdateLevel_QuitLeague) == false) {
			jsResult["ret"] = 1;
			return true;
		}
		if (getClub()->getBaseData()->isNotJoinedLeague(nLeagueID)) {
			jsResult["ret"] = 2;
			return true;
		}
		jsResult["ret"] = 0;
		return true;
	}

	if (eAsync_league_Dismiss_Check == nRequestType) {
		auto nUserID = jsReqContent["uid"].asUInt();
		auto nLeagueID = jsReqContent["leagueID"].asUInt();

		if (checkUpdateLevel(nUserID, eClubUpdateLevel_DismissLeague) == false) {
			jsResult["ret"] = 1;
			return true;
		}
		if (getClub()->getBaseData()->isNotCreatedLeague(nLeagueID)) {
			jsResult["ret"] = 2;
			return true;
		}
		jsResult["ret"] = 0;
		return true;
	}

	if (eAsync_league_FireClub_Check == nRequestType) {
		auto nUserID = jsReqContent["uid"].asUInt();
		auto nLeagueID = jsReqContent["leagueID"].asUInt();
		if (checkUpdateLevel(nUserID, eClubUpdateLevel_FireFromLeague) == false) {
			jsResult["ret"] = 1;
			return true;
		}
		if (getClub()->getBaseData()->isNotJoinOrCreateLeague(nLeagueID)) {
			jsResult["ret"] = 2;
			return true;
		}
		jsResult["ret"] = 0;
		return true;
	}

	if (eAsync_league_TreatEvent == nRequestType) {
		auto nUserID = jsReqContent["uid"].asUInt();
		auto nLeagueID = jsReqContent["leagueID"].asUInt();
		auto nRequireLevel = jsReqContent["level"].asUInt();
		if (isNotJoin(nUserID)) {
			jsResult["ret"] = 1;
			return true;
		}
		if (nLeagueID == 0 || getClub()->getBaseData()->isNotJoinOrCreateLeague(nLeagueID)) {
			jsResult["ret"] = 2;
			return true;
		}
		if (checkUpdateLevel(nUserID, nRequireLevel) == false) {
			jsResult["ret"] = 3;
			return true;
		}
		jsResult["ret"] = 0;
		return true;
	}
	return false;
}

bool CClubMemberData::onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID) {
	if (eAsync_Club_T_Player_Check == nRequestType) {
		auto nLeagueID = jsReqContent["leagueID"].asUInt();
		if (nLeagueID == 0) {
			return false;
		}
		else {
			auto nUserID = jsReqContent["uid"].asUInt();
			//auto pAsync = getClub()->getClubMgr()->getSvrApp()->getAsynReqQueue();
			auto pApp = getClub()->getClubMgr()->getSvrApp();
			Json::Value jsReq;
			jsReq = jsReqContent;
			//jsReq["leagueID"] = nLeagueID;
			//jsReq["clubID"] = getClub()->getClubID();
			//jsReq["uid"] = nUserID;
			//jsReq["tuid"] = jsReqContent["tuid"];
			//getClub()->getClubMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, nLeagueID, eAsync_league_CreateRoom_Check, );
			pApp->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, nLeagueID, eAsync_League_T_Player_Check, jsReq, [pApp, nSenderID, nReqSerial, nSenderPort, this, nLeagueID, nUserID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
			{
				Json::Value jsRet;
				if (isTimeOut)
				{
					LOGFMTE(" request of league time out uid = %u , can not check T player ", nUserID);
					jsRet["ret"] = 7;
					pApp->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsRet, getClub()->getClubID());
					return;
				}

				uint8_t nReqRet = retContent["ret"].asUInt();
				uint8_t nRet = 0;
				do {
					if (0 != nReqRet)
					{
						nRet = 3;
						break;
					}


				} while (0);

				jsRet["ret"] = nRet;
				pApp->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsRet, getClub()->getClubID());
			});
		}
		return true;
	}
	return false;
}

bool CClubMemberData::addMember(uint32_t nMemberUID, uint8_t nLevel) {
	if (isNotJoin(nMemberUID) == false) {
		LOGFMTE("Club add member error, is already the member! ClubID = %u, MemberUID = %u", getClub()->getClubID(), nMemberUID);
		return false;
	}

	if (nLevel > eClubMemberLevel_Creator) {
		LOGFMTE("Club add member error, level is out of use! ClubID = %u, MemberUID = %u, level = %u", getClub()->getClubID(), nMemberUID, nLevel);
		return false;
	}

	if (nLevel == eClubMemberLevel_Creator && findCreator()) {
		LOGFMTE("Club add member error, can only have one owner! ClubID = %u, MemberUID = %u, level = %u", getClub()->getClubID(), nMemberUID, nLevel);
		return false;
	}

	if (getClub()->getMemberLimit() && getMemberCnt() >= getClub()->getMemberLimit()) {
		LOGFMTE("Club add member error, can not add more member! ClubID = %u, MemberUID = %u, level = %u", getClub()->getClubID(), nMemberUID, nLevel);
		return false;
	}

	auto it = m_mMembers.find(nMemberUID);
	if (it == m_mMembers.end()) {
		stMemberBaseData stmbd;
		stmbd.nMemberUID = nMemberUID;
		stmbd.nState = eClubState_Normal;
		stmbd.nLevel = nLevel;
		stmbd.nJoinTime = time(NULL);
		stmbd.nQuitTime = 0;

		if (std::find(m_vMemberAddIDs.begin(), m_vMemberAddIDs.end(), nMemberUID) == m_vMemberAddIDs.end()) {
			m_vMemberAddIDs.push_back(nMemberUID);
		}
		
		m_mMembers[nMemberUID] = stmbd;
	}
	else {
		it->second.nState = eClubState_Normal;
		it->second.nJoinTime = time(NULL);
		it->second.nLevel = nLevel;
		it->second.nQuitTime = 0;

		m_vMemberDertyIDs.push_back(nMemberUID);
	}

	return true;
}

bool CClubMemberData::isNotJoin(uint32_t uMemberUID) {
	auto it = m_mMembers.find(uMemberUID);
	return it == m_mMembers.end() || it->second.nState == eClubState_Delete;
}

uint16_t CClubMemberData::getMemberCnt() {
	uint16_t nCnt = 0;
	for (auto& ref : m_mMembers) {
		if (ref.second.nState == eClubState_Delete) {
			continue;
		}
		nCnt++;
	}
	return nCnt;
}

void CClubMemberData::memberDataToJson(Json::Value& jsData) {
	std::vector<stMemberBaseData*> vMembers;
	for (auto& ref : m_mMembers) {
		if (ref.second.nState != eClubState_Delete) {
			vMembers.push_back(&(ref.second));
			/*Json::Value jsMember;
			jsMember["id"] = ref.first;
			jsMember["level"] = ref.second.nLevel;
			jsData[jsData.size()] = jsMember;*/
		}
	}
	std::sort(vMembers.begin(), vMembers.end(), [](stMemberBaseData* st1, stMemberBaseData* st2) {
		if (st1->nLevel != st2->nLevel) {
			return st1->nLevel > st2->nLevel;
		}
		else {
			return st1->nJoinTime < st2->nJoinTime;
		}
	});

	for (auto& ref : vMembers) {
		Json::Value jsMember;
		jsMember["id"] = ref->nMemberUID;
		jsMember["level"] = ref->nLevel;
		jsMember["remark"] = ref->cRemark;
		jsData[jsData.size()] = jsMember;
	}
}

bool CClubMemberData::checkUpdateLevel(uint32_t nMemberID, uint8_t nLevelRequired, bool canEquals) {
	//LOGFMTE("level = %u, rLevel = %u", getMemberLevel(nMemberID), nLevelRequired);
	if (canEquals) {
		return getMemberLevel(nMemberID) >= nLevelRequired;
	}
	else {
		return getMemberLevel(nMemberID) > nLevelRequired;
	}
}

uint8_t CClubMemberData::getMemberLevel(uint32_t nMemberID) {
	auto it = m_mMembers.find(nMemberID);
	if (it == m_mMembers.end() || it->second.nState == eClubState_Delete) {
		return 0;
	}
	return m_mMembers[nMemberID].nLevel;
}

char* CClubMemberData::getMemberRemark(uint32_t nMemberID) {
	auto it = m_mMembers.find(nMemberID);
	if (it == m_mMembers.end() || it->second.nState == eClubState_Delete) {
		return nullptr;
	}
	return m_mMembers[nMemberID].cRemark;
}

bool CClubMemberData::grantFoundation(uint32_t nGrantUID, uint32_t nMemberUID, uint32_t nAmount) {
	if (getClub()->getFoundation() < nAmount) {
		return false;
	}

	getClub()->addFoundation(-1 * (int32_t)nAmount);
	Json::Value jsMsg;
	jsMsg["targetUID"] = nMemberUID;
	jsMsg["clubID"] = getClub()->getClubID();
	jsMsg["agentID"] = nGrantUID;
	jsMsg["amount"] = nAmount;
	getClub()->getClubMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, nMemberUID, eAsync_Club_AddCoin, jsMsg);
	return true;
}

void CClubMemberData::pushAsyncRequestToAll(eMsgPort nPortID, eAsyncReq nReqType, Json::Value& jsData) {
	auto pAsync = getClub()->getClubMgr()->getSvrApp()->getAsynReqQueue();
	for (auto& ref : m_mMembers) {
		if (ref.second.nState == eClubState_Delete) {
			continue;
		}
		jsData["targetUID"] = ref.second.nMemberUID;
		pAsync->pushAsyncRequest(nPortID, ref.second.nMemberUID, nReqType, jsData);
	}
}

void CClubMemberData::pushAsyncRequestToLevelNeed(eMsgPort nPortID, eAsyncReq nReqType, Json::Value& jsData, uint8_t nLevel) {
	auto pAsync = getClub()->getClubMgr()->getSvrApp()->getAsynReqQueue();
	for (auto& ref : m_mMembers) {
		if (ref.second.nState == eClubState_Delete || ref.second.nLevel < nLevel) {
			continue;
		}
		jsData["targetUID"] = ref.second.nMemberUID;
		pAsync->pushAsyncRequest(nPortID, ref.second.nMemberUID, nReqType, jsData);
	}
}

bool CClubMemberData::fireMember(uint32_t nMemberUID) {
	auto it = m_mMembers.find(nMemberUID);
	if (it == m_mMembers.end() || it->second.nState == eClubState_Delete) {
		return false;
	}
	it->second.nState = eClubState_Delete;
	it->second.nQuitTime = time(NULL);
	m_vMemberDertyIDs.push_back(nMemberUID);
	return true;
}

void CClubMemberData::timerSave() {
	if (m_bReadingDB) {
		return;
	}

	if (m_vMemberDertyIDs.empty() && m_vMemberAddIDs.empty()) {
		return;
	}
	
	for (auto nAddUID : m_vMemberAddIDs) {
		if (m_mMembers.find(nAddUID) == m_mMembers.end()) {
			LOGFMTE("caution: add club member info error, can not find member info, clubID = %u , memberUID = %u, caution!!!!!", getClub()->getClubID(), nAddUID);
			continue;
		}
		auto info = m_mMembers[nAddUID];
		Json::Value jssql;
		char pBuffer[512] = { 0 };
		sprintf_s(pBuffer, "insert into clubmember (clubID, userUID, state, level, joinDataTime) values (%u, %u, %u, %u, from_unixtime( %u ));", getClub()->getClubID(), info.nMemberUID, info.nState, info.nLevel, info.nJoinTime);
		jssql["sql"] = pBuffer;
		auto pReqQueue = m_pClub->getClubMgr()->getSvrApp()->getAsynReqQueue();
		pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, getClub()->getClubID(), eAsync_DB_Add, jssql);
	}
	m_vMemberAddIDs.clear();

	for (auto nDirtyUID : m_vMemberDertyIDs) {
		if (m_mMembers.find(nDirtyUID) == m_mMembers.end()) {
			LOGFMTE("caution: save club member info error, can not find member info, clubID = %u , memberUID = %u, caution!!!!!", getClub()->getClubID(), nDirtyUID);
			continue;
		}
		auto info = m_mMembers[nDirtyUID];
		Json::Value jssql;
		char pBuffer[512] = { 0 };
		sprintf_s(pBuffer, "update clubmember set state = %u, level = %u, quitDataTime = from_unixtime( %u ), remark = '%s' where clubID = %u and userUID = %u;", info.nState, info.nLevel, info.nQuitTime, info.cRemark, getClub()->getClubID(), nDirtyUID);
		jssql["sql"] = pBuffer;
		auto pReqQueue = m_pClub->getClubMgr()->getSvrApp()->getAsynReqQueue();
		pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, getClub()->getClubID(), eAsync_DB_Update, jssql);
	}
	m_vMemberDertyIDs.clear();
}

void CClubMemberData::readMemberFormDB(uint32_t nOffset) {
	m_bReadingDB = true;
	std::ostringstream ss;
	ss << "SELECT userUID, state, level, unix_timestamp(joinDataTime) as joinDataTime, unix_timestamp(quitDataTime) as quitDataTime, remark FROM clubmember where clubID = " << (UINT)m_pClub->getClubID() << " order by userUID desc limit 10 offset " << (UINT)nOffset << ";";
	Json::Value jsReq;
	jsReq["sql"] = ss.str();
	m_pClub->getClubMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB, rand(), eAsync_DB_Select, jsReq, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut) {
		if (isTimeOut)
		{
			LOGFMTE("caution: load club member from DB time out, clubID = %u , caution!!!!!", getClub()->getClubID());
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
			auto nUserUID = jsRow["userUID"].asUInt();
			auto iter = m_mMembers.find(nUserUID);
			if (iter != m_mMembers.end())
			{
				LOGFMTE("bug: why already have this member uid = %u , in club id = %u double read , bug!!!!!!", nUserUID, getClub()->getClubID());
				doProcessAfterReadDB();
				return;
			}

			stMemberBaseData stMemberInfo;
			stMemberInfo.nMemberUID = nUserUID;
			stMemberInfo.nState = jsRow["state"].asUInt();
			stMemberInfo.nLevel = jsRow["level"].asUInt();
			stMemberInfo.nJoinTime = jsRow["joinDataTime"].asUInt();
			stMemberInfo.nQuitTime = jsRow["quitDataTime"].asUInt();
			stMemberInfo.setRemark(jsRow["remark"].asCString());

			m_mMembers[nUserUID] = stMemberInfo;
		}

		if (nAft < 10)  // only read one page ;
		{
			doProcessAfterReadDB();
			return;
		}

		// not finish , go on read 
		auto nSize = m_mMembers.size();
		readMemberFormDB(nSize);
	});
}

void CClubMemberData::doProcessAfterReadDB() {
	m_bReadingDB = false;
}

bool CClubMemberData::findCreator() {
	auto it = m_mMembers.find(getClub()->getCreatorUID());
	return it != m_mMembers.end() && it->second.nState != eClubState_Delete;
}