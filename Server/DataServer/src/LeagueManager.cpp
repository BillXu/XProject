#include "LeagueManager.h"
#include "League.h"
#include "DataServerApp.h"
#include "AsyncRequestQuene.h"
#include "log4z.h"
#include "ClubManager.h"
#include "Utility.h"
#include "Club.h"
#include <time.h>
#include "ClubMemberData.h"
CLeagueManager::CLeagueManager() {
	m_vAllLeagues.clear();
}

CLeagueManager::~CLeagueManager() {
	auto iter = m_vAllLeagues.begin();
	for (; iter != m_vAllLeagues.end(); ++iter)
	{
		iter->second->onTimerSave();
		delete iter->second;
		iter->second = nullptr;
	}
	m_vAllLeagues.clear();
}

void CLeagueManager::init(IServerApp* svrApp)
{
	IGlobalModule::init(svrApp);
	readLeagueFormDB();
}

void CLeagueManager::onConnectedSvr(bool isReconnected)
{
	//return;
	if (isReconnected)
	{
		return;
	}
	auto pSync = getSvrApp()->getAsynReqQueue();
	Json::Value jsReq;
	jsReq["sql"] = "SELECT max(leagueID) as maxLeagueID FROM league;";
	pSync->pushAsyncRequest(ID_MSG_PORT_DB, getSvrApp()->getCurSvrIdx(), eAsync_DB_Select, jsReq, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut) {
		if (isTimeOut)
		{
			return;
		}
		uint32_t nAft = retContent["afctRow"].asUInt();
		auto jsData = retContent["data"];
		if (nAft == 0 || jsData.isNull())
		{
			LOGFMTW("read maxLeagueID error, but no matter ");
			m_nMaxLeagueID = getSvrApp()->getCurSvrIdx() + getSvrApp()->getCurSvrMaxCnt();
			return;
		}

		auto jsRow = jsData[(uint32_t)0];
		m_nMaxLeagueID = jsRow["maxLeagueID"].asUInt() + getSvrApp()->getCurSvrMaxCnt();
		m_nMaxLeagueID -= m_nMaxLeagueID % getSvrApp()->getCurSvrMaxCnt();
		m_nMaxLeagueID += getSvrApp()->getCurSvrIdx();
#ifdef  _DEBUG
		//m_nMaxClubID = 0;
#endif //  _DEBUG
		LOGFMTD("maxLeagueID = %u", m_nMaxLeagueID);
	});

	jsReq["sql"] = "SELECT max(eventID) as maxEventID FROM leagueevent;";
	pSync->pushAsyncRequest(ID_MSG_PORT_DB, getSvrApp()->getCurSvrIdx(), eAsync_DB_Select, jsReq, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut) {
		if (isTimeOut)
		{
			return;
		}
		uint32_t nAft = retContent["afctRow"].asUInt();
		auto jsData = retContent["data"];
		if (nAft == 0 || jsData.isNull())
		{
			LOGFMTW("read league maxEventID id error, but no matter ");
			m_nMaxEventID = getSvrApp()->getCurSvrIdx() + getSvrApp()->getCurSvrMaxCnt();
			return;
		}

		auto jsRow = jsData[(uint32_t)0];
		m_nMaxEventID = jsRow["maxEventID"].asUInt() + getSvrApp()->getCurSvrMaxCnt();
		m_nMaxEventID -= m_nMaxEventID % getSvrApp()->getCurSvrMaxCnt();
		m_nMaxEventID += getSvrApp()->getCurSvrIdx();
#ifdef  _DEBUG
		//m_nMaxClubID = 0;
#endif //  _DEBUG
		LOGFMTD("league maxEventID = %u", m_nMaxEventID);
	});
}

void CLeagueManager::onExit()
{
	/*auto iter = m_vAllClubs.begin();
	for (; iter != m_vAllClubs.end(); ++iter)
	{
	iter->second->onRelease();
	}*/
	IGlobalModule::onExit();
}

void CLeagueManager::sendMsgToClient(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nSessionID) {
	sendMsg(prealMsg, nMsgType, 0, nSessionID);
}

bool CLeagueManager::onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID)
{
	if (MSG_LEAGUE_CREATE_LEAGUE == nMsgType) {
		auto pClubMgr = ((DataServerApp*)getSvrApp())->getClubMgr();
		auto pClub = pClubMgr->getClubByClubID(nTargetID);
		if (pClub) {
			if (prealMsg["uid"].isNull() || prealMsg["uid"].isUInt() == false) {
				LOGFMTE("can not find uid to create league club id = %d, from = %d", nTargetID, nSenderID);
				return true;
			}
			auto nUserID = prealMsg["uid"].asUInt();
			if (pClub->getClubMemberData()->checkUpdateLevel(nUserID, eClubUpdateLevel_CreateLeague) == false) {
				LOGFMTE("can not create league from club id = %d, because user id = %u do not have enough level from = %d", nTargetID, nUserID, nSenderID);
				return true;
			}
			createLeague(pClub, prealMsg, nSenderID);
		}
		else {
			LOGFMTE("can not find club id = %d to create league, from = %d", nTargetID, nSenderID);
		}
		return true;
	}

	auto pTargetLeague = getLeagueByLeagueID(nTargetID);
	if (pTargetLeague && pTargetLeague->getState() != eClubState_Delete && pTargetLeague->onMsg(prealMsg, nMsgType, eSenderPort, nSenderID))
	{
		return true;
	}
	else
	{
		if (pTargetLeague == NULL)
		{
			LOGFMTE("can not find league id = %d to process msg id = %d ,from = %d", nTargetID, nMsgType, nSenderID);
		}
		else
		{
			LOGFMTE("unprocess msg for league id = %d , msg = %d ,from %d ", nTargetID, nMsgType, nSenderID);
		}
	}
	return false;
}

bool CLeagueManager::onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)
{
	// common requst ;
	switch (nRequestType)
	{
		//case eAsync_Inform_Player_LeavedRoom:
		//case eAsync_Request_EnterRoomInfo:
		//case eAsync_Request_CreateRoomInfo:
		//case eAsync_Inform_CreatedRoom:
		//case eAsync_Inform_RoomDeleted:
		//{
		//	auto nUID = jsReqContent["targetUID"].asUInt();
		//	auto pPlayer = getPlayerByUserUID(nUID);
		//	if (!pPlayer)
		//	{
		//		LOGFMTE(" can not find player uid = %u , to process async req = %u, let is time out", nUID, nRequestType);
		//		jsResult["ret"] = 2;
		//		break;
		//	}
		//	return pPlayer->onAsyncRequest(nRequestType, jsReqContent, jsResult);
		//}
		//break;
	default:
	{
		if (jsReqContent["leagueID"].isNull() == false)
		{
			auto nLeagueID = jsReqContent["leagueID"].asUInt();
			auto pLeague = getLeagueByLeagueID(nLeagueID);
			if (pLeague == nullptr)
			{
				//jsResult["ret"] = 1; // can not find target player ;
				return false;
			}
			else
			{
				return pLeague->onAsyncRequest(nRequestType, jsReqContent, jsResult);
			}
		}
		else {
			return false;
		}
	}

	}
	return true;
}

CLeague* CLeagueManager::getLeagueByLeagueID(uint32_t nLeagueID) {
	auto iter = m_vAllLeagues.find(nLeagueID);
	if (iter != m_vAllLeagues.end()) {
		return iter->second;
	}
	return nullptr;
}

bool CLeagueManager::onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID, uint16_t nTargetID) {
	return false;
}

void CLeagueManager::update(float fDeta) {
	IGlobalModule::update(fDeta);
}

void CLeagueManager::onTimeSave() {
	for (auto& ref : m_vAllLeagues)
	{
		if (ref.second)
		{
			ref.second->onTimerSave();
		}
	}
}

void CLeagueManager::addActiveLeague(CLeague* pLeague) {
	if (!pLeague)
	{
		LOGFMTE("Can not Add NULL club !");
		return;
	}
	auto iter = m_vAllLeagues.find(pLeague->getLeagueID());
	if (iter != m_vAllLeagues.end())
	{
		LOGFMTE("league to add had existed in active map ! , league ID = %d", pLeague->getLeagueID());
		delete iter->second;
		iter->second = NULL;
		m_vAllLeagues.erase(iter);
	}
	m_vAllLeagues[pLeague->getLeagueID()] = pLeague;
}

void CLeagueManager::readLeagueFormDB(uint32_t nOffset) {
	//return;
	m_bReadingDB = true;
	std::ostringstream ss;
	ss << "SELECT leagueID, creator, name, headIcon, unix_timestamp(createTime) as createTime, state, memberLimit, joinLimit FROM league where state !=  " << eClubState_Delete << " order by leagueID desc limit 10 offset " << nOffset << ";";
	Json::Value jsReq;
	jsReq["sql"] = ss.str();
	getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB, rand(), eAsync_DB_Select, jsReq, [this, nOffset](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut) {
		if (isTimeOut)
		{
			LOGFMTE("caution: load league info from DB time out , caution!!!!!");
			doProcessAfterReadDB();
			return;
		}

		uint32_t nAft = retContent["afctRow"].asUInt();
		LOGFMTE("Attention: finish read league info, count = %u", nAft);
		auto jsData = retContent["data"];
		if (nAft == 0 || jsData.isNull())
		{
			doProcessAfterReadDB();
			return;
		}

		for (uint32_t nRowIdx = 0; nRowIdx < nAft; ++nRowIdx)
		{
			auto jsRow = jsData[nRowIdx];
			uint32_t nLeagueID = jsRow["leagueID"].asUInt();

			if (getSvrApp()->isIDInThisSvr(nLeagueID) == false) {
				LOGFMTI("Attention: Filter this league in this DataServer, leagueID = %u", nLeagueID);
				continue;
			}

			auto iter = m_vAllLeagues.find(nLeagueID);
			if (iter != m_vAllLeagues.end())
			{
				LOGFMTE("bug: why already have this league id = %u, double read , bug!!!!!!", nLeagueID);
				doProcessAfterReadDB();
				return;
			}

			auto pLeague = new CLeague();
			pLeague->setLeagueID(nLeagueID);
			pLeague->setCreatorCID(jsRow["creator"].asUInt());
			pLeague->setName(jsRow["name"].asCString());
			pLeague->setIcon(jsRow["headIcon"].asCString());
			pLeague->setCreateTime(jsRow["createTime"].asUInt());
			pLeague->setState(jsRow["state"].asUInt());
			pLeague->setMemberLimit(jsRow["memberLimit"].asUInt());
			pLeague->setJoinLimit(jsRow["joinLimit"].asUInt());

			pLeague->init(this);
			m_vAllLeagues[nLeagueID] = pLeague;
		}

		if (nAft < 10)  // only read one page ;
		{
			doProcessAfterReadDB();
			return;
		}

		// not finish , go on read 
		//auto nSize = m_vAllClubs.size();
		readLeagueFormDB(nOffset + 10);
	});
}

void CLeagueManager::doProcessAfterReadDB() {
	m_bReadingDB = false;
}

uint32_t CLeagueManager::generateLeagueID()
{
	m_nMaxLeagueID += getSvrApp()->getCurSvrMaxCnt();
	return m_nMaxLeagueID;
}

uint32_t CLeagueManager::generateEventID() {
	m_nMaxEventID += getSvrApp()->getCurSvrMaxCnt();
	return m_nMaxEventID;
}

void CLeagueManager::createLeague(CClub* pClub, const Json::Value& jsReqContent, uint32_t nSenderID) {
	Json::Value jsResult;
	std::string sLeagueName = jsReqContent["name"].asCString();
	parseESSpace(sLeagueName);
	if (sLeagueName.empty()) {
		jsResult["ret"] = 1;
		sendMsgToClient(jsResult, MSG_CLUB_CREATE_CLUB, nSenderID);
		LOGFMTE("Create league error, name is null! clubID = %u", pClub->getClubID());
	}
	auto nLeagueID = generateLeagueID();
	if (m_vAllLeagues.find(nLeagueID) != m_vAllLeagues.end()) {
		jsResult["ret"] = 3;
		sendMsgToClient(jsResult, MSG_CLUB_CREATE_CLUB, nSenderID);
		LOGFMTE("Create league error, bug, generateID is in used! clubID = %u", pClub->getClubID());
	}

	CLeague* pLeague = new CLeague();
	pLeague->setLeagueID(nLeagueID);
	pLeague->setCreateTime(time(NULL));
	pLeague->setCreatorCID(pClub->getClubID());
	pLeague->setIcon(jsReqContent["icon"].asCString());
	pLeague->setState(eClubState_Normal);
	pLeague->setName(sLeagueName.c_str());
	pLeague->setMemberLimit(DEFAULT_LEAGUE_MEMBER_LIMIT);
	pLeague->setJoinLimit(eLeagueJoinLimit_None);

	pLeague->init(this);
	pLeague->insertIntoDB();
	m_vAllLeagues[nLeagueID] = pLeague;

	pClub->addCreatedLeague(nLeagueID);

	jsResult["ret"] = 0;
	sendMsgToClient(jsResult, MSG_LEAGUE_CREATE_LEAGUE, nSenderID);
	LOGFMTE("club create league success, clubID = %u, leagueID = %u", pClub->getClubID(), nLeagueID);
}