#include "LeagueMemberData.h"
#include "log4z.h"
#include "League.h"
#include "LeagueManager.h"
#include "ISeverApp.h"
#include "AsyncRequestQuene.h"
#include <time.h>
CLeagueMemberData::CLeagueMemberData()
{
	m_eType = eLeagueComponent_MemberData;
	m_bReadingDB = false;
	m_vMemberDertyCIDs.clear();
	m_vMemberAddCIDs.clear();
	m_mMembers.clear();
}

CLeagueMemberData::~CLeagueMemberData() {

}

void CLeagueMemberData::init(CLeague* pLeague) {
	ILeagueComponent::init(pLeague);
	readMemberFormDB();
}

void CLeagueMemberData::reset() {
	m_bReadingDB = false;
	m_vMemberDertyCIDs.clear();
	m_vMemberAddCIDs.clear();
	m_mMembers.clear();
}

bool CLeagueMemberData::onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort, uint32_t nSenderID) {
	/*if (MSG_LEAGUE_JOIN_LEAGUE == nmsgType) {
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

		Json::Value jsReq;
		jsReq["uid"] = nUserID;
		jsReq["leagueID"] = getLeague()->getLeagueID();
		auto pAsync = getLeague()->getLeagueMgr()->getSvrApp()->getAsynReqQueue();
		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nClubID, eAsync_league_JoinLeague, jsReq, [pAsync, nSenderID, this, nUserID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
		{
			if (isTimeOut)
			{
				LOGFMTE(" request time out uid = %u , can not join league id = %u ", nUserID, getLeague()->getLeagueID());
				Json::Value jsRet;
				jsRet["ret"] = 7;
				sendMsgToClient(jsRet, MSG_LEAGUE_JOIN_LEAGUE, nSenderID);
				return;
			}


		}, recvValue, nUserID);

		return true;
	}*/
	return false;
}

bool CLeagueMemberData::onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult) {
	if (eAsync_league_clubRoom_Back_Integration == nRequestType) {
		auto nClubID = jsReqContent["clubID"].asUInt();
		if (isNotJoin(nClubID)) {
			return true;
		}
		auto nAmount = jsReqContent["chip"].asUInt();
		auto nRoomID = jsReqContent["roomID"].asUInt();
		auto nUID = jsReqContent["uid"].asUInt();
		addIntegration(nClubID, nAmount);
		LOGFMTI("player id = %u, back integration from room id = %u to league id = %u by club id = %u of amount = %u", nUID, nRoomID, getLeague()->getLeagueID(), nClubID, nAmount);
		return true;
	}

	if (eAsync_league_apply_Club_Integration == nRequestType) {
		if (jsReqContent["clubID"].isNull() || jsReqContent["clubID"].isUInt() == false) {
			jsResult["ret"] = 1;
			return true;
		}
		auto nClubID = jsReqContent["clubID"].asUInt();
		if (isNotJoin(nClubID)) {
			jsResult["ret"] = 2;
			return true;
		}

		jsResult["ret"] = 0;
		auto& st = m_mMembers[nClubID];
		Json::Value jsDetail, jsInfo;
		jsDetail["leagueID"] = getLeague()->getLeagueID();
		jsDetail["integration"] = st.nIntegration - st.nTempIntegration;
		jsDetail["initialIntegration"] = st.nInitialIntegration;
		jsInfo = jsReqContent["info"];
		jsInfo[jsInfo.size()] = jsDetail;
		jsResult["info"] = jsInfo;
		return true;
	}

	if (eAsync_league_apply_DragIn_Clubs == nRequestType) {
		if (jsReqContent["clubIDs"].isNull() || jsReqContent["clubIDs"].isArray() == false) {
			jsResult["ret"] = 1;
			return true;
		}
		Json::Value jsMsg, jsClubs;
		for (auto& ref : jsReqContent["clubIDs"]) {
			auto nClubID = ref.asUInt();
			if (isNotJoin(nClubID)) {
				continue;
			}
			else {
				jsClubs[jsClubs.size()] = nClubID;
			}
		}
		jsResult["ret"] = 0;
		jsResult["clubIDs"] = jsClubs;
		return true;
	}

	if (eAsync_league_CreateRoom_Check == nRequestType) {
		auto nClubID = jsReqContent["clubID"].asUInt();
		if (nClubID) {
			if (checkUpdateLevel(nClubID, eLeagueMemberLevel_Creator)) {
				jsResult["ret"] = 0;
			}
			else {
				jsResult["ret"] = 2;
				LOGFMTE("League create room error, club can not create room! leagueID = %u, MemberCID = %u", getLeague()->getLeagueID(), nClubID);
			}
		}
		else {
			jsResult["ret"] = 1;
			LOGFMTE("League create room error, clubID is missing! leagueID = %u", getLeague()->getLeagueID());
		}
		return true;
	}
	return false;
}

bool CLeagueMemberData::addMember(uint32_t nMemberCID, uint8_t nLevel) {
	if (isNotJoin(nMemberCID) == false) {
		LOGFMTE("League add member error, is already have the member! leagueID = %u, MemberCID = %u", getLeague()->getLeagueID(), nMemberCID);
		return false;
	}

	if (nLevel > eLeagueMemberLevel_Creator) {
		LOGFMTE("League add member error, level is out of use! LeaugeID = %u, MemberCID = %u, level = %u", getLeague()->getLeagueID(), nMemberCID, nLevel);
		return false;
	}

	if (nLevel == eLeagueMemberLevel_Creator && findCreator()) {
		LOGFMTE("League add member error, can only have one owner! LeagueID = %u, MemberCID = %u, level = %u", getLeague()->getLeagueID(), nMemberCID, nLevel);
		return false;
	}

	if (getLeague()->getMemberLimit() && getMemberCnt() >= getLeague()->getMemberLimit()) {
		LOGFMTE("League add member error, can not add more member! LeagueID = %u, MemberCID = %u, level = %u", getLeague()->getLeagueID(), nMemberCID, nLevel);
		return false;
	}

	auto it = m_mMembers.find(nMemberCID);
	if (it == m_mMembers.end()) {
		stMemberBaseData stmbd;
		stmbd.nMemberCID = nMemberCID;
		stmbd.nState = eClubState_Normal;
		stmbd.nLevel = nLevel;
		stmbd.nJoinTime = time(NULL);
		stmbd.nQuitTime = 0;

		if (std::find(m_vMemberAddCIDs.begin(), m_vMemberAddCIDs.end(), nMemberCID) == m_vMemberAddCIDs.end()) {
			m_vMemberAddCIDs.push_back(nMemberCID);
		}
		
		m_mMembers[nMemberCID] = stmbd;
	}
	else {
		it->second.nState = eClubState_Normal;
		it->second.nJoinTime = time(NULL);
		it->second.nLevel = nLevel;
		it->second.nQuitTime = 0;

		if (isNotDirty(nMemberCID)) {
			m_vMemberDertyCIDs.push_back(nMemberCID);
		}
	}

	return true;
}

bool CLeagueMemberData::isNotJoin(uint32_t uMemberCID) {
	auto it = m_mMembers.find(uMemberCID);
	return it == m_mMembers.end() || it->second.nState == eClubState_Delete;
}

uint16_t CLeagueMemberData::getMemberCnt() {
	uint16_t nCnt = 0;
	for (auto& ref : m_mMembers) {
		if (ref.second.nState != eClubState_Delete) {
			nCnt++;
		}
	}
	return nCnt;
}

void CLeagueMemberData::memberDataToJson(Json::Value& jsData) {
	for (auto& ref : m_mMembers) {
		if (ref.second.nState != eClubState_Delete) {
			Json::Value jsMember;
			jsMember["id"] = ref.first;
			jsMember["level"] = ref.second.nLevel;
			jsData[jsData.size()] = jsMember;
		}
	}
}

void CLeagueMemberData::memberIDToJson(Json::Value& jsData) {
	for (auto& ref : m_mMembers) {
		if (ref.second.nState != eClubState_Delete) {
			Json::Value jsMember;
			jsMember["id"] = ref.first;
			jsData[jsData.size()] = jsMember;
		}
	}
}

bool CLeagueMemberData::checkUpdateLevel(uint32_t nMemberCID, uint8_t nLevelRequired) {
	return getMemberLevel(nMemberCID) >= nLevelRequired;
}

uint8_t CLeagueMemberData::getMemberLevel(uint32_t nMemberCID) {
	auto it = m_mMembers.find(nMemberCID);
	if (it == m_mMembers.end() || it->second.nState == eClubState_Delete) {
		return 0;
	}
	return m_mMembers[nMemberCID].nLevel;
}

bool CLeagueMemberData::grantIntegration(uint32_t nGrantCID, uint32_t nMemberCID, uint32_t nAmount) {
	Json::Value jsMsg;
	jsMsg["clubID"] = nMemberCID;
	jsMsg["leagueID"] = getLeague()->getLeagueID();
	jsMsg["agentID"] = nGrantCID;
	jsMsg["amount"] = nAmount;
	getLeague()->getLeagueMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, nMemberCID, eAsync_League_AddIntegration, jsMsg);
	return true;
}

bool CLeagueMemberData::addIntegration(uint32_t nGrantCID, int32_t nAmount) {
	if (isNotJoin(nGrantCID)) {
		return false;
	}

	auto& it = m_mMembers[nGrantCID];

	if (it.nTempIntegration && nAmount < 0) {
		int32_t nTemp = it.nIntegration + nAmount - (int32_t)it.nTempIntegration;
		if (nTemp < 0) {
			return false;
		}
	}

	it.nIntegration += nAmount;
	if (isNotDirty(nGrantCID)) {
		m_vMemberDertyCIDs.push_back(nGrantCID);
	}
	return true;
}

bool CLeagueMemberData::setStopState(uint32_t nMemberCID, uint8_t& nState) {
	if (isNotJoin(nMemberCID)) {
		return false;
	}

	nState = nState == 0 ? 0 : 1;

	auto& it = m_mMembers[nMemberCID];
	if (it.nStop != nState) {
		it.nStop = nState;
		if (isNotDirty(nMemberCID)) {
			m_vMemberDertyCIDs.push_back(nMemberCID);
		}
	}
	return true;
}

uint8_t CLeagueMemberData::getStropState(uint32_t nMemberCID) {
	if (isNotJoin(nMemberCID)) {
		return -1;
	}

	auto& it = m_mMembers[nMemberCID];
	return it.nStop;
}

bool CLeagueMemberData::addInitialIntegration(uint32_t nGrantCID, int32_t nAmount) {
	if (isNotJoin(nGrantCID)) {
		return false;
	}

	auto& it = m_mMembers[nGrantCID];
	it.nInitialIntegration += nAmount;
	if (isNotDirty(nGrantCID)) {
		m_vMemberDertyCIDs.push_back(nGrantCID);
	}
	return true;
}

int32_t CLeagueMemberData::getIntegration(uint32_t nMemberCID) {
	if (isNotJoin(nMemberCID)) {
		return 0;
	}

	auto& it = m_mMembers[nMemberCID];
	return it.nIntegration - it.nTempIntegration;
}

int32_t CLeagueMemberData::getInitialIntegration(uint32_t nMemberCID) {
	if (isNotJoin(nMemberCID)) {
		return 0;
	}

	return m_mMembers[nMemberCID].nInitialIntegration;
}

bool CLeagueMemberData::checkDecreaseIntegration(uint32_t nMemberCID, uint32_t nAmount) {
	if (isNotJoin(nMemberCID)) {
		return false;
	}

	auto& it = m_mMembers[nMemberCID];
	
	if (it.nStop) {
		return false;
	}

	if (it.nIntegration < it.nTempIntegration + nAmount) {
		return false;
	}
	else {
		it.nTempIntegration += nAmount;
		return true;
	}
	//return it.nIntegration >= nAmount;
}

bool CLeagueMemberData::decreaseIntegration(uint32_t nMemberCID, uint32_t nAmount) {
	if (isNotJoin(nMemberCID)) {
		return false;
	}

	auto& it = m_mMembers[nMemberCID];
	if (it.nIntegration < nAmount) {
		return false;
	}

	it.nIntegration -= nAmount;
	if (it.nTempIntegration < nAmount) {
		it.nTempIntegration = 0;
	}
	else {
		it.nTempIntegration -= nAmount;
	}
	
	if (isNotDirty(nMemberCID)) {
		m_vMemberDertyCIDs.push_back(nMemberCID);
	}
	return true;
}

bool CLeagueMemberData::clearTempIntegration(uint32_t nMemberCID, uint32_t nAmount) {
	if (isNotJoin(nMemberCID)) {
		return false;
	}

	auto& it = m_mMembers[nMemberCID];
	if (it.nTempIntegration < nAmount) {
		it.nTempIntegration = 0;
	}
	else {
		it.nTempIntegration -= nAmount;
	}
	return true;
}

bool CLeagueMemberData::fireClub(uint32_t nMemberCID, uint32_t nFireCID) {
	auto nFirelevel = getMemberLevel(nFireCID);
	auto nMemberLevel = getMemberLevel(nMemberCID);
	if (nMemberLevel > nFirelevel && nMemberCID != nFireCID) {
		m_mMembers[nFireCID].nState = eClubState_Delete;
		m_mMembers[nFireCID].nQuitTime = time(NULL);
		if (isNotDirty(nFireCID)) {
			m_vMemberDertyCIDs.push_back(nFireCID);
		}
		return true;
	}
	return false;
}

bool CLeagueMemberData::onClubQuit(uint32_t nMemberCID) {
	auto it = m_mMembers.find(nMemberCID);
	if (it == m_mMembers.end() || it->second.nState == eClubState_Delete) {
		return false;
	}
	it->second.nState = eClubState_Delete;
	it->second.nQuitTime = time(NULL);
	if (isNotDirty(nMemberCID)) {
		m_vMemberDertyCIDs.push_back(nMemberCID);
	}
	return true;
}

void CLeagueMemberData::pushAsyncRequestToAll(eMsgPort nPortID, eAsyncReq nReqType, Json::Value& jsData) {
	auto pAsync = getLeague()->getLeagueMgr()->getSvrApp()->getAsynReqQueue();
	for (auto& ref : m_mMembers) {
		if (ref.second.nState == eClubState_Delete) {
			continue;
		}
		jsData["clubID"] = ref.second.nMemberCID;
		pAsync->pushAsyncRequest(nPortID, ref.second.nMemberCID, nReqType, jsData);
	}
}

void CLeagueMemberData::pushAsyncRequestToLevelNeed(eMsgPort nPortID, eAsyncReq nReqType, Json::Value& jsData, uint8_t nLevel) {
	auto pAsync = getLeague()->getLeagueMgr()->getSvrApp()->getAsynReqQueue();
	for (auto& ref : m_mMembers) {
		if (ref.second.nState == eClubState_Delete || ref.second.nLevel < nLevel) {
			continue;
		}
		jsData["clubID"] = ref.second.nMemberCID;
		pAsync->pushAsyncRequest(nPortID, ref.second.nMemberCID, nReqType, jsData);
	}
}

void CLeagueMemberData::timerSave() {
	if (m_bReadingDB) {
		return;
	}

	if (m_vMemberDertyCIDs.empty() && m_vMemberAddCIDs.empty()) {
		return;
	}

	for (auto nAddCID : m_vMemberAddCIDs) {
		if (m_mMembers.find(nAddCID) == m_mMembers.end()) {
			LOGFMTE("caution: add league member info error, can not find member info, leagueID = %u , memberCID = %u, caution!!!!!", getLeague()->getLeagueID(), nAddCID);
			continue;
		}
		auto info = m_mMembers[nAddCID];
		Json::Value jssql;
		char pBuffer[512] = { 0 };
		sprintf_s(pBuffer, "insert into leaguemember (leagueID, clubID, state, level, joinDataTime) values (%u, %u, %u, %u, from_unixtime( %u ));", getLeague()->getLeagueID(), info.nMemberCID, info.nState, info.nLevel, info.nJoinTime);
		jssql["sql"] = pBuffer;
		auto pReqQueue = m_pLeague->getLeagueMgr()->getSvrApp()->getAsynReqQueue();
		pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, getLeague()->getLeagueID(), eAsync_DB_Add, jssql);
	}
	m_vMemberAddCIDs.clear();

	for (auto nDirtyCID : m_vMemberDertyCIDs) {
		if (m_mMembers.find(nDirtyCID) == m_mMembers.end()) {
			LOGFMTE("caution: save league member info error, can not find member info, leagueID = %u , memberCID = %u, caution!!!!!", getLeague()->getLeagueID(), nDirtyCID);
			continue;
		}
		auto info = m_mMembers[nDirtyCID];
		Json::Value jssql;
		char pBuffer[512] = { 0 };
		sprintf_s(pBuffer, "update leaguemember set state = %u, level = %u, quitDataTime = from_unixtime( %u ), integration = %d, initialIntegration = %d, stop = %u where leagueID = %u and clubID = %u;", info.nState, info.nLevel, info.nQuitTime, info.nIntegration, info.nInitialIntegration, info.nStop, getLeague()->getLeagueID(), nDirtyCID);
		jssql["sql"] = pBuffer;
		auto pReqQueue = m_pLeague->getLeagueMgr()->getSvrApp()->getAsynReqQueue();
		pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, getLeague()->getLeagueID(), eAsync_DB_Update, jssql);
	}
	m_vMemberDertyCIDs.clear();
}

void CLeagueMemberData::readMemberFormDB(uint32_t nOffset) {
	m_bReadingDB = true;
	std::ostringstream ss;
	ss << "SELECT clubID, state, level, unix_timestamp(joinDataTime) as joinDataTime, unix_timestamp(quitDataTime) as quitDataTime, integration, initialIntegration, stop FROM leaguemember where leagueID = " << getLeague()->getLeagueID() << " order by clubID desc limit 10 offset " << (UINT)nOffset << ";";
	Json::Value jsReq;
	jsReq["sql"] = ss.str();
	getLeague()->getLeagueMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB, rand(), eAsync_DB_Select, jsReq, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut) {
		if (isTimeOut)
		{
			LOGFMTE("caution: load league member from DB time out, leagueID = %u , caution!!!!!", getLeague()->getLeagueID());
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
			auto nMemberCID = jsRow["clubID"].asUInt();
			auto iter = m_mMembers.find(nMemberCID);
			if (iter != m_mMembers.end())
			{
				LOGFMTE("bug: why already have this member cid = %u , in league id = %u double read , bug!!!!!!", nMemberCID, getLeague()->getLeagueID());
				doProcessAfterReadDB();
				return;
			}

			stMemberBaseData stMemberInfo;
			stMemberInfo.nMemberCID = nMemberCID;
			stMemberInfo.nState = jsRow["state"].asUInt();
			stMemberInfo.nLevel = jsRow["level"].asUInt();
			stMemberInfo.nJoinTime = jsRow["joinDataTime"].asUInt();
			stMemberInfo.nQuitTime = jsRow["quitDataTime"].asUInt();
			stMemberInfo.nIntegration = jsRow["integration"].asInt();
			stMemberInfo.nInitialIntegration = jsRow["initialIntegration"].asInt();
			stMemberInfo.nStop = jsRow["stop"].asUInt();

			m_mMembers[nMemberCID] = stMemberInfo;
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

void CLeagueMemberData::doProcessAfterReadDB() {
	m_bReadingDB = false;
}

bool CLeagueMemberData::findCreator() {
	auto it = m_mMembers.find(getLeague()->getCreatorCID());
	return it != m_mMembers.end() && it->second.nState != eClubState_Delete;
}

bool CLeagueMemberData::isNotDirty(uint32_t nMemberCID) {
	return std::find(m_vMemberDertyCIDs.begin(), m_vMemberDertyCIDs.end(), nMemberCID) == m_vMemberDertyCIDs.end();
}