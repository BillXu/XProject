#include "ClubManager.h"
#include "Club.h"
#include "log4z.h"
#include "ClubDefine.h"
#include "AsyncRequestQuene.h"
#include "DataServerApp.h"
#include "Player.h"
#include "PlayerBaseData.h"
#include "Utility.h"
#include <time.h>

CClubManager::CClubManager() {
	m_vAllClubs.clear();
}

CClubManager::~CClubManager() {
	auto iter = m_vAllClubs.begin();
	for (; iter != m_vAllClubs.end(); ++iter)
	{
		iter->second->onTimerSave();
		delete iter->second;
		iter->second = nullptr;
	}
	m_vAllClubs.clear();
}

void CClubManager::init(IServerApp* svrApp)
{
	IGlobalModule::init(svrApp);
	readClubFormDB();
}

void CClubManager::onConnectedSvr(bool isReconnected)
{
	//return;
	if (isReconnected)
	{
		return;
	}
	auto pSync = getSvrApp()->getAsynReqQueue();
	Json::Value jsReq;
	jsReq["sql"] = "SELECT max(clubID) as maxClubID FROM club;";
	pSync->pushAsyncRequest(ID_MSG_PORT_DB, getSvrApp()->getCurSvrIdx(), eAsync_DB_Select, jsReq, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut) {
		if (isTimeOut)
		{
			return;
		}
		uint32_t nAft = retContent["afctRow"].asUInt();
		auto jsData = retContent["data"];
		if (nAft == 0 || jsData.isNull())
		{
			LOGFMTW("read maxClubID error, but no matter ");
			m_nMaxClubID = getSvrApp()->getCurSvrIdx() + getSvrApp()->getCurSvrMaxCnt();
			return;
		}

		auto jsRow = jsData[(uint32_t)0];
		m_nMaxClubID = jsRow["maxClubID"].asUInt() + getSvrApp()->getCurSvrMaxCnt();
		m_nMaxClubID -= m_nMaxClubID % getSvrApp()->getCurSvrMaxCnt();
		m_nMaxClubID += getSvrApp()->getCurSvrIdx();
#ifdef  _DEBUG
		//m_nMaxClubID = 0;
#endif //  _DEBUG
		LOGFMTD("maxClubID = %u", m_nMaxClubID);
	});

	jsReq["sql"] = "SELECT max(eventID) as maxEventID FROM clubevent;";
	pSync->pushAsyncRequest(ID_MSG_PORT_DB, getSvrApp()->getCurSvrIdx(), eAsync_DB_Select, jsReq, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut) {
		if (isTimeOut)
		{
			return;
		}
		uint32_t nAft = retContent["afctRow"].asUInt();
		auto jsData = retContent["data"];
		if (nAft == 0 || jsData.isNull())
		{
			LOGFMTW("read maxEventID id error, but no matter ");
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
		LOGFMTD("maxEventID = %u", m_nMaxEventID);
	});
}

void CClubManager::onExit()
{
	/*auto iter = m_vAllClubs.begin();
	for (; iter != m_vAllClubs.end(); ++iter)
	{
		iter->second->onRelease();
	}*/
	IGlobalModule::onExit();
}

void CClubManager::sendMsgToClient(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nSessionID) {
	sendMsg(prealMsg, nMsgType, 0, nSessionID);
}

bool CClubManager::onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID)
{
	if (MSG_CLUB_CREATE_CLUB == nMsgType) {
		auto pPlayerMgr = ((DataServerApp*)getSvrApp())->getPlayerMgr();
		auto pPlayer = pPlayerMgr->getPlayerByUserUID(nTargetID);
		if (pPlayer) {
			createClub(pPlayer, prealMsg);
		}
		return true;
	}

	auto pTargetClub = getClubByClubID(nTargetID);
	if (pTargetClub && pTargetClub->getState() != eClubState_Delete && pTargetClub->onMsg(prealMsg, nMsgType, eSenderPort, nSenderID))
	{
		return true;
	}
	else
	{
		if (pTargetClub == NULL)
		{
			LOGFMTE("can not find club id = %d to process msg id = %d ,from = %d", nTargetID, nMsgType, nSenderID);
		}
		else
		{
			LOGFMTE("unprocess msg for club id = %d , msg = %d ,from %d ", nTargetID, nMsgType, nSenderID);
		}
	}
	return false;
}

bool CClubManager::onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)
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
		if (jsReqContent["clubID"].isNull() == false)
		{
			auto nClubID = jsReqContent["clubID"].asUInt();
			auto pClub = getClubByClubID(nClubID);
			if (pClub == nullptr)
			{
				jsResult["ret"] = 1; // can not find target player ;
				return false;
			}
			else
			{
				return pClub->onAsyncRequest(nRequestType, jsReqContent, jsResult);
			}
		}
		else {
			return false;
		}
	}

	}
	return true;
}

bool CClubManager::onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID, uint16_t nTargetID) {
	switch (nRequestType)
	{
		
	default:
	{
		if (jsReqContent["clubID"].isNull() == false)
		{
			auto nClubID = jsReqContent["clubID"].asUInt();
			auto pClub = getClubByClubID(nClubID);
			if (pClub == nullptr)
			{
				return false;
			}
			else
			{
				return pClub->onAsyncRequestDelayResp(nRequestType, nReqSerial, jsReqContent, nSenderPort, nSenderID);
			}
		}
		else {
			return false;
		}
	}

	}
	return true;
}

CClub* CClubManager::getClubByClubID(uint32_t nClubID) {
	auto iter = m_vAllClubs.find(nClubID);
	if (iter != m_vAllClubs.end()) {
		return iter->second;
	}
	return nullptr;
}

void CClubManager::update(float fDeta) {
	IGlobalModule::update(fDeta);
}

void CClubManager::onTimeSave() {
	for (auto& ref : m_vAllClubs)
	{
		if (ref.second)
		{
			ref.second->onTimerSave();
		}
	}
}

void CClubManager::addActiveClub(CClub* pClub) {
	if (!pClub)
	{
		LOGFMTE("Can not Add NULL club !");
		return;
	}
	auto iter = m_vAllClubs.find(pClub->getClubID());
	if (iter != m_vAllClubs.end())
	{
		LOGFMTE("club to add had existed in active map ! , club ID = %d", pClub->getClubID());
		delete iter->second;
		iter->second = NULL;
		m_vAllClubs.erase(iter);
	}
	m_vAllClubs[pClub->getClubID()] = pClub;
}

void CClubManager::readClubFormDB(uint32_t nOffset) {
	//return;
	m_bReadingDB = true;
	std::ostringstream ss;
	ss << "SELECT clubID, creator, name, headIcon, unix_timestamp(createTime) as createTime, region, state, memberLimit, foundation, integration, description, createRoomType, searchLimit, joinedLeague, createdLeague FROM xproject.club where state !=  " << eClubState_Delete << " order by clubID desc limit 10 offset " << nOffset << ";";
	Json::Value jsReq;
	jsReq["sql"] = ss.str();
	//printf("%s", ss.str().c_str());
	getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB, 0, eAsync_DB_Select, jsReq, [this, nOffset](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut) {
		if (isTimeOut)
		{
			LOGFMTE("caution: load club info from DB time out , caution!!!!!");
			doProcessAfterReadDB();
			return;
		}

		uint32_t nAft = retContent["afctRow"].asUInt();
		LOGFMTE("Attention: finish read club info, count = %u", nAft);
		auto jsData = retContent["data"];
		if (nAft == 0 || jsData.isNull())
		{
			doProcessAfterReadDB();
			return;
		}

		for (uint32_t nRowIdx = 0; nRowIdx < nAft; ++nRowIdx)
		{
			auto jsRow = jsData[nRowIdx];
			uint32_t nClubID = jsRow["clubID"].asUInt();

			if (getSvrApp()->isIDInThisSvr(nClubID) == false) {
				LOGFMTI("Attention: Filter this club in this DataServer, clubID = %u", nClubID);
				continue;
			}

			auto iter = m_vAllClubs.find(nClubID);
			if (iter != m_vAllClubs.end())
			{
				LOGFMTE("bug: why already have this club id = %u, double read , bug!!!!!!", nClubID);
				doProcessAfterReadDB();
				return;
			}

			auto pClub = new CClub();
			pClub->setClubID(nClubID);
			pClub->setCreatorUID(jsRow["creator"].asUInt());
			pClub->setName(jsRow["name"].asCString());
			pClub->setIcon(jsRow["headIcon"].asCString());
			pClub->setCreateTime(jsRow["createTime"].asUInt());
			pClub->setRegion(jsRow["region"].asCString());
			pClub->setState(jsRow["state"].asUInt());
			pClub->setMemberLimit(jsRow["memberLimit"].asUInt());
			pClub->setFoundation(jsRow["foundation"].asUInt());
			pClub->setIntegration(jsRow["integration"].asUInt());
			pClub->setCreateRoomType(jsRow["createRoomType"].asUInt());
			pClub->setSearchLimit(jsRow["searchLimit"].asUInt());
			pClub->setDescription(jsRow["description"].asCString());

			std::string sJoinedLeague = jsRow["joinedLeague"].asCString();
			if (sJoinedLeague.size()) {
				VEC_STRING vsJoinedLeague;
				StringSplit(sJoinedLeague, ".", vsJoinedLeague);
				for (auto sLeague : vsJoinedLeague) {
					pClub->getBaseData()->vJoinedLeague.push_back((uint32_t)std::stoi(sLeague));
				}
			}

			std::string sCreatedLeague = jsRow["createdLeague"].asCString();
			if (sCreatedLeague.size()) {
				VEC_STRING vsCreatedLeague;
				StringSplit(sCreatedLeague, ".", vsCreatedLeague);
				for (auto sLeague : vsCreatedLeague) {
					pClub->getBaseData()->vCreatedLeague.push_back((uint32_t)std::stoi(sLeague));
				}
			}

			pClub->init(this);
			m_vAllClubs[nClubID] = pClub;
		}

		if (nAft < 10)  // only read one page ;
		{
			doProcessAfterReadDB();
			return;
		}

		// not finish , go on read 
		//auto nSize = m_vAllClubs.size();
		readClubFormDB(nOffset + 10);
	});
}

void CClubManager::doProcessAfterReadDB() {
	m_bReadingDB = false;
}

uint32_t CClubManager::generateClubID()
{
	m_nMaxClubID += getSvrApp()->getCurSvrMaxCnt();
	return m_nMaxClubID;
}

uint32_t CClubManager::generateEventID() {
	m_nMaxEventID += getSvrApp()->getCurSvrMaxCnt();
	return m_nMaxEventID;
}

void CClubManager::createClub(CPlayer* pPlayer, const Json::Value& jsReqContent) {
	Json::Value jsResult;
	std::string sClubName = jsReqContent["name"].asCString();
	parseESSpace(sClubName);
	if (sClubName.empty()) {
		jsResult["ret"] = 1;
		pPlayer->sendMsgToClient(jsResult, MSG_CLUB_CREATE_CLUB);
		LOGFMTE("Create club error, name is null! playerID = %u", pPlayer->getUserUID());
	}
	std::string sRegion = jsReqContent["region"].asCString();
	parseESSpace(sRegion);
	if (sRegion.empty()) {
		jsResult["ret"] = 2;
		pPlayer->sendMsgToClient(jsResult, MSG_CLUB_CREATE_CLUB);
		LOGFMTE("Create club error, regaion is null! playerID = %u", pPlayer->getUserUID());
	}
	auto nClubID = generateClubID();
	if (m_vAllClubs.find(nClubID) != m_vAllClubs.end()) {
		jsResult["ret"] = 3;
		pPlayer->sendMsgToClient(jsResult, MSG_CLUB_CREATE_CLUB);
		LOGFMTE("Create club error, bug, generateID is in used! playerID = %u", pPlayer->getUserUID());
	}

	std::string sDescription = jsReqContent["description"].asCString();
	parseESSpace(sDescription);

	CClub* pClub = new CClub();
	pClub->setClubID(nClubID);
	pClub->setCreateTime(time(NULL));
	pClub->setCreatorUID(pPlayer->getUserUID());
	pClub->setIcon(jsReqContent["icon"].asCString());
	pClub->setState(eClubState_Normal);
	pClub->setName(sClubName.c_str());
	pClub->setRegion(sRegion.c_str());
	pClub->setMemberLimit(DEFAULT_CLUB_MEMBER_LIMIT);
	pClub->setFoundation(0);
	pClub->setIntegration(0);
	pClub->setCreateRoomType(eClubCreateRoom_Admin);
	pClub->setSearchLimit(eClubSearchLimit_None);
	pClub->setDescription(sDescription.c_str());

	pClub->init(this);
	pClub->insertIntoDB();
	m_vAllClubs[nClubID] = pClub;

	pPlayer->getBaseData()->addCreatedClub(nClubID);

	jsResult["ret"] = 0;
	jsResult["clubID"] = nClubID;
	pPlayer->sendMsgToClient(jsResult, MSG_CLUB_CREATE_CLUB);
	LOGFMTE("player create club success, playerUID = %u, clubID = %u", pPlayer->getUserUID(), pClub->getClubID());
}