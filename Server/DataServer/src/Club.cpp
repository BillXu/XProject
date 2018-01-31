#include "ClubDefine.h"
#include "ClubMemberdata.h"
#include "ClubGamedata.h"
#include "ClubEvent.h"
#include "Club.h"
#include "ClubManager.h"
#include <string>
#include "ISeverApp.h"
#include "AsyncRequestQuene.h"
#include "Utility.h"
#include "log4z.h"

CClub::CClub() {
	m_stBaseData.zeroReset();
	//memset(&m_stBaseData, 0, sizeof(m_stBaseData));
	m_bBaseDataDirty = false;
	m_bMoneyDataDirty = false;
	m_bUseFulDataDirty = false;
	for (int i = eClubComponent_None; i < eClubComponent_Max; ++i)
	{
		m_vAllComponents[i] = NULL;
	}
}

CClub::~CClub() {
	for (int i = eClubComponent_None; i < eClubComponent_Max; ++i)
	{
		auto p = m_vAllComponents[i];
		if (p)
			delete p;
		m_vAllComponents[i] = NULL;
	}
}

void CClub::init(CClubManager* pClubManager) {
	m_pClubManager = pClubManager;
	m_vAllComponents[eClubComponent_MemberData] = new CClubMemberData();
	m_vAllComponents[eClubComponent_GameData] = new CClubGameData();
	m_vAllComponents[eClubComponent_Event] = new CClubEvent();
	for (int i = eClubComponent_None; i < eClubComponent_Max; ++i)
	{
		auto p = m_vAllComponents[i];
		if (p)
		{
			p->init(this);
		}
	}
}

void CClub::reset() {
	for (int i = eClubComponent_None; i < eClubComponent_Max; ++i)
	{
		auto p = m_vAllComponents[i];
		if (p)
		{
			p->reset();
		}
	}
	m_stBaseData.zeroReset();
}

bool CClub::onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort, uint32_t nSenderID) {
	if (MSG_LEAGUE_CLUB_LEAGUE_INFO == nmsgType) {
		Json::Value jsMsg, jsJoined, jsCreated;
		for (auto& ref : m_stBaseData.vJoinedLeague) {
			jsJoined[jsJoined.size()] = ref;
		}
		for (auto& ref : m_stBaseData.vCreatedLeague) {
			jsCreated[jsCreated.size()] = ref;
		}
		jsMsg["joined"] = jsJoined;
		jsMsg["created"] = jsCreated;
		sendMsgToClient(jsMsg, nmsgType, nSenderID);
		return true;
	}
	if (MSG_CLUB_APPLY_CLUB_INFO == nmsgType) {
		Json::Value jsInfo;
		jsInfo["clubID"] = m_stBaseData.nClubID;
		jsInfo["name"] = m_stBaseData.cName;
		jsInfo["creator"] = m_stBaseData.nCreatorUID;
		jsInfo["nom"] = getClubMemberData()->getMemberCnt();
		jsInfo["lom"] = m_stBaseData.nMemberLimit;
		jsInfo["region"] = m_stBaseData.cRegion;
		jsInfo["description"] = m_stBaseData.cDescription;
		jsInfo["icon"] = m_stBaseData.cHeadiconUrl;
		jsInfo["nor"] = getClubGameData()->getRoomCnt();
		jsInfo["ret"] = 0;
		sendMsgToClient(jsInfo, nmsgType, nSenderID);
		return true;
	}

	if (MSG_CLUB_APPLY_CLUB_DETAIL == nmsgType) {
		Json::Value jsMsg, jsMembers;
		jsMsg["clubID"] = m_stBaseData.nClubID;
		jsMsg["creator"] = m_stBaseData.nCreatorUID;
		getClubMemberData()->memberDataToJson(jsMembers);
		jsMsg["members"] = jsMembers;
		jsMsg["createType"] = m_stBaseData.nCreateRoomType;
		jsMsg["searchLimit"] = m_stBaseData.nSearchLimit;
		jsMsg["foundation"] = m_stBaseData.nFoundation;
		jsMsg["ret"] = 0;
		sendMsgToClient(jsMsg, nmsgType, nSenderID);
		return true;
	}

	if (MSG_CLUB_INFO_UPDATE_ICON == nmsgType) {
		Json::Value jsMsg;
		uint32_t nUserID = recvValue["uid"].isUInt() ? recvValue["uid"].asUInt() : 0;
		if (nUserID == 0) {
			jsMsg["ret"] = 1;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club icon error, userID is miss, clubID = %u", getClubID());
			return true;
		}
		if (getClubMemberData()->checkUpdateLevel(nUserID, eClubUpdateLevel_Icon) == false) {
			jsMsg["ret"] = 2;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club icon error, user level is not enough, clubID = %u, userID = %u", getClubID(), nUserID);
			return true;
		}
		auto cIcon = recvValue["icon"].asCString();
		setIcon(cIcon);
		m_bBaseDataDirty = true;
		jsMsg["ret"] = 0;
		jsMsg["clubID"] = getClubID();
		sendMsgToClient(jsMsg, nmsgType, nSenderID);
		return true;
	}

	if (MSG_CLUB_INFO_UPDATE_NAME == nmsgType) {
		Json::Value jsMsg;
		uint32_t nUserID = recvValue["uid"].isUInt() ? recvValue["uid"].asUInt() : 0;
		if (nUserID == 0) {
			jsMsg["ret"] = 1;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club name error, userID is miss, clubID = %u", getClubID());
			return true;
		}
		if (getClubMemberData()->checkUpdateLevel(nUserID, eClubUpdateLevel_Name) == false) {
			jsMsg["ret"] = 2;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club name error, user level is not enough, clubID = %u, userID = %u", getClubID(), nUserID);
			return true;
		}
		std::string cName = recvValue["name"].asCString();
		parseESSpace(cName);
		if (cName.size() == 0) {
			jsMsg["ret"] = 3;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club name error, name is null, clubID = %u", getClubID());
			return true;
		}
		setName(cName.c_str());
		m_bBaseDataDirty = true;
		jsMsg["ret"] = 0;
		jsMsg["clubID"] = getClubID();
		sendMsgToClient(jsMsg, nmsgType, nSenderID);
		return true;
	}

	if (MSG_CLUB_INFO_UPDATE_CREATE_TYPE == nmsgType) {
		Json::Value jsMsg;
		uint32_t nUserID = recvValue["uid"].isUInt() ? recvValue["uid"].asUInt() : 0;
		if (nUserID == 0) {
			jsMsg["ret"] = 1;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club create type error, userID is miss, clubID = %u", getClubID());
			return true;
		}
		if (getClubMemberData()->checkUpdateLevel(nUserID, eClubUpdateLevel_CreateType) == false) {
			jsMsg["ret"] = 2;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club create type error, user level is not enough, clubID = %u, userID = %u", getClubID(), nUserID);
			return true;
		}
		if (recvValue["state"].isNull() || recvValue["state"].isUInt() == false) {
			jsMsg["ret"] = 3;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club create type error, change state is miss, clubID = %u, userID = %u", getClubID(), nUserID);
			return true;
		}
		uint8_t nState = recvValue["state"].asUInt();
		if (nState > eClubCreateRoom_Creator) {
			jsMsg["ret"] = 4;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club create type error, change state is not allowed, clubID = %u, userID = %u, state = %u", getClubID(), nUserID, nState);
			return true;
		}
		setCreateRoomType(nState);
		m_bUseFulDataDirty = true;
		jsMsg["ret"] = 0;
		jsMsg["clubID"] = getClubID();
		jsMsg["state"] = nState;
		sendMsgToClient(jsMsg, nmsgType, nSenderID);
		return true;
	}

	if (MSG_CLUB_INFO_UPDATE_SEARCH_LIMIT == nmsgType) {
		Json::Value jsMsg;
		uint32_t nUserID = recvValue["uid"].isUInt() ? recvValue["uid"].asUInt() : 0;
		if (nUserID == 0) {
			jsMsg["ret"] = 1;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club search limit error, userID is miss, clubID = %u", getClubID());
			return true;
		}
		if (getClubMemberData()->checkUpdateLevel(nUserID, eClubUpdateLevel_SearchLimit) == false) {
			jsMsg["ret"] = 2;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club search limit error, user level is not enough, clubID = %u, userID = %u", getClubID(), nUserID);
			return true;
		}
		if (recvValue["state"].isNull() || recvValue["state"].isUInt() == false) {
			jsMsg["ret"] = 3;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club search limit error, change state is miss, clubID = %u, userID = %u", getClubID(), nUserID);
			return true;
		}
		uint8_t nState = recvValue["state"].asUInt();
		if (nState > eClubSearchLimit_All) {
			jsMsg["ret"] = 4;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club search limit error, change state is not allowed, clubID = %u, userID = %u, state = %u", getClubID(), nUserID, nState);
			return true;
		}
		setSearchLimit(nState);
		m_bUseFulDataDirty = true;
		jsMsg["ret"] = 0;
		jsMsg["clubID"] = getClubID();
		jsMsg["state"] = nState;
		sendMsgToClient(jsMsg, nmsgType, nSenderID);
		return true;
	}

	if (MSG_CLUB_INFO_UPDATE_DESCRIPTION == nmsgType) {
		Json::Value jsMsg;
		uint32_t nUserID = recvValue["uid"].isUInt() ? recvValue["uid"].asUInt() : 0;
		if (nUserID == 0) {
			jsMsg["ret"] = 1;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club description error, userID is miss, clubID = %u", getClubID());
			return true;
		}
		if (getClubMemberData()->checkUpdateLevel(nUserID, eClubUpdateLevel_Description) == false) {
			jsMsg["ret"] = 2;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club description error, user level is not enough, clubID = %u, userID = %u", getClubID(), nUserID);
			return true;
		}
		std::string cDescription = recvValue["description"].asCString();
		parseESSpace(cDescription);
		if (cDescription.size() == 0) {
			jsMsg["ret"] = 3;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			LOGFMTE("Update club description error, description is null, clubID = %u", getClubID());
			return true;
		}
		setDescription(cDescription.c_str());
		m_bBaseDataDirty = true;
		jsMsg["ret"] = 0;
		jsMsg["clubID"] = getClubID();
		sendMsgToClient(jsMsg, nmsgType, nSenderID);
		return true;

		return true;
	}


	for (int i = eClubComponent_None; i < eClubComponent_Max; ++i)
	{
		IClubComponent* p = m_vAllComponents[i];
		if (p)
		{
			if (p->onMsg(recvValue, nmsgType, eSenderPort, nSenderID))
			{
				return true;
			}
		}
	}

	//LOGFMTE("Unprocessed msg id = %d, from = %d  uid = %d",pMsg->usMsgType,eSenderPort,getUserUID() ) ;
	return false;
}

bool CClub::onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)
{
	if (eAsync_league_ClubQuit == nRequestType) {
		auto nLeagueID = jsReqContent["leagueID"].asUInt();
		auto nUserID = jsReqContent["uid"].asUInt();
		m_stBaseData.eraseLeague(nLeagueID);
		return true;
	}

	if (eAsync_league_Dismiss == nRequestType) {
		auto nLeagueID = jsReqContent["leagueID"].asUInt();
		auto nUserID = jsReqContent["uid"].asUInt();
		m_stBaseData.eraseLeague(nLeagueID);
		return true;
	}

	if (eAsync_league_FireClub == nRequestType) {
		auto nLeagueID = jsReqContent["leagueID"].asUInt();
		auto nClubID = jsReqContent["agentCID"].asUInt();
		auto nUserID = jsReqContent["uid"].asUInt();
		m_stBaseData.eraseJoinedLeague(nLeagueID);
		return true;
	}

	if (eAsync_league_ClubJoin == nRequestType) {
		auto nLeagueID = jsReqContent["leagueID"].asUInt();
		auto nAgentUID = jsReqContent["agentID"].asUInt();
		if (nLeagueID && m_stBaseData.isNotJoinOrCreateLeague(nLeagueID)) {
			m_stBaseData.vJoinedLeague.push_back(nLeagueID);
			m_bLeagueDataDirty = true;
		}
		return true;
	}

	if (eAsync_league_JoinLeague == nRequestType) {
		auto nUserID = jsReqContent["uid"].asUInt();
		auto nLeagueID = jsReqContent["leagueID"].asUInt();
		if (nLeagueID == 0) {
			jsResult["ret"] = 1;
			return true;
		}
		if (m_stBaseData.isNotJoinOrCreateLeague(nLeagueID) == false) {
			jsResult["ret"] = 2;
			return true;
		}
		if (getClubMemberData()->checkUpdateLevel(nUserID, eClubUpdateLevel_JoinLeague) == false) {
			jsResult["ret"] = 3;
			return true;
		}
		
		jsResult["ret"] = 0;
		return true;
	}

	for (int i = eClubComponent_None; i < eClubComponent_Max; ++i)
	{
		IClubComponent* p = m_vAllComponents[i];
		if (p)
		{
			if (p->onAsyncRequest(nRequestType, jsReqContent, jsResult))
			{
				return true;
			}
		}
	}
	return false;
}

bool CClub::onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID) {
	for (int i = eClubComponent_None; i < eClubComponent_Max; ++i)
	{
		IClubComponent* p = m_vAllComponents[i];
		if (p)
		{
			if (p->onAsyncRequestDelayResp(nRequestType, nReqSerial, jsReqContent, nSenderPort, nSenderID))
			{
				return true;
			}
		}
	}
	return false;
}

IClubComponent* CClub::getComponent(eClubComponentType eType) {
	if (eType < eClubComponent_Max) {
		return m_vAllComponents[eType];
	}
	return nullptr;
}

void CClub::onTimerSave() {
	if (m_pClubManager->isReadingDB()) {
		return;
	}

	if (m_bBaseDataDirty) {
		m_bBaseDataDirty = false;
		Json::Value jssql;
		char pBuffer[512] = { 0 };
		sprintf_s(pBuffer, "update club set name = '%s', headIcon = '%s', description = '%s' where clubID = %u;", m_stBaseData.cName, m_stBaseData.cHeadiconUrl, m_stBaseData.cDescription, m_stBaseData.nClubID);
		std::string str = pBuffer;
		jssql["sql"] = pBuffer;
		auto pReqQueue = m_pClubManager->getSvrApp()->getAsynReqQueue();
		pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, m_stBaseData.nClubID, eAsync_DB_Update, jssql);
	}

	if (m_bMoneyDataDirty) {
		m_bMoneyDataDirty = false;
		Json::Value jssql;
		char pBuffer[512] = { 0 };
		sprintf_s(pBuffer, "update club set foundation = %u, integration = %u where clubID = %u;", m_stBaseData.nFoundation, m_stBaseData.nIntegration, m_stBaseData.nClubID);
		std::string str = pBuffer;
		jssql["sql"] = pBuffer;
		auto pReqQueue = m_pClubManager->getSvrApp()->getAsynReqQueue();
		pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, m_stBaseData.nClubID, eAsync_DB_Update, jssql);
	}

	if (m_bLevelInfoDirty) {
		m_bLevelInfoDirty = false;
		Json::Value jssql;
		char pBuffer[512] = { 0 };
		sprintf_s(pBuffer, "update club set state = %u, memberLimit = %u where clubID = %u;", m_stBaseData.nState, m_stBaseData.nMemberLimit, m_stBaseData.nClubID);
		std::string str = pBuffer;
		jssql["sql"] = pBuffer;
		auto pReqQueue = m_pClubManager->getSvrApp()->getAsynReqQueue();
		pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, m_stBaseData.nClubID, eAsync_DB_Update, jssql);
	}

	if (m_bUseFulDataDirty) {
		m_bUseFulDataDirty = false;
		Json::Value jssql;
		char pBuffer[512] = { 0 };
		sprintf_s(pBuffer, "update club set createRoomType = %u, searchLimit = %u where clubID = %u;", m_stBaseData.nCreateRoomType, m_stBaseData.nSearchLimit, m_stBaseData.nClubID);
		std::string str = pBuffer;
		jssql["sql"] = pBuffer;
		auto pReqQueue = m_pClubManager->getSvrApp()->getAsynReqQueue();
		pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, m_stBaseData.nClubID, eAsync_DB_Update, jssql);
	}

	if (m_bLeagueDataDirty) {
		m_bLeagueDataDirty = false;

		Json::Value jssql;
		char pBuffer[512] = { 0 };
		auto sJoined = m_stBaseData.jlToString();
		auto sCreated = m_stBaseData.clToString();
		sprintf_s(pBuffer, "update club set joinedLeague = '%s' ,createdLeague = '%s' where clubID = %u;", sJoined.c_str(), sCreated.c_str(), getClubID());
		std::string str = pBuffer;
		jssql["sql"] = pBuffer;
		auto pReqQueue = getClubMgr()->getSvrApp()->getAsynReqQueue();
		pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, getClubID(), eAsync_DB_Update, jssql);
		LOGFMTD("clubID = %u save joinedLeague = %s , createdLeague = %s", getClubID(), sJoined.c_str(), sCreated.c_str());
	}

	for (int i = eClubComponent_None; i < eClubComponent_Max; ++i)
	{
		auto p = m_vAllComponents[i];
		if (p)
		{
			p->timerSave();
		}
	}
}

void CClub::sendMsgToClient(Json::Value& jsMsg, uint16_t nMsgType, uint32_t nSessionID) {
	getClubMgr()->sendMsg(jsMsg, nMsgType, getClubID(), nSessionID);
}

void CClub::setClubID(uint32_t nClubID) {
	m_stBaseData.nClubID = nClubID;
}

void CClub::setCreatorUID(uint32_t nCreatorID) {
	m_stBaseData.nCreatorUID = nCreatorID;
}

void CClub::setName(const char* cName) {
	sprintf_s(m_stBaseData.cName, "%s", cName);
}

void CClub::setIcon(const char* cIcon) {
	sprintf_s(m_stBaseData.cHeadiconUrl, "%s", cIcon);
}

void CClub::setCreateTime(uint32_t nTime) {
	m_stBaseData.nCreateTime = nTime;
}

void CClub::setRegion(const char* cRegion) {
	sprintf_s(m_stBaseData.cRegion, "%s", cRegion);
}

void CClub::setState(uint8_t nState) {
	m_stBaseData.nState = nState;
}

void CClub::setMemberLimit(uint16_t nMemberLimit) {
	m_stBaseData.nMemberLimit = nMemberLimit;
}

void CClub::setFoundation(uint32_t nFoundation) {
	m_stBaseData.nFoundation = nFoundation;
}

void CClub::setIntegration(uint32_t nIntegration) {
	m_stBaseData.nIntegration = nIntegration;
}

void CClub::setCreateRoomType(uint8_t nCreateRoomType) {
	m_stBaseData.nCreateRoomType = nCreateRoomType;
}
void CClub::setSearchLimit(uint8_t nSearchLimit) {
	m_stBaseData.nSearchLimit = nSearchLimit;
}

void CClub::setDescription(const char* cDescription) {
	sprintf_s(m_stBaseData.cDescription, "%s", cDescription);
}

void CClub::addFoundation(int32_t nAmount) {
	if (nAmount < 0 && abs(nAmount) > m_stBaseData.nFoundation) {
		m_stBaseData.nFoundation = 0;
	}
	else {
		m_stBaseData.nFoundation += nAmount;
	}
	m_bMoneyDataDirty = true;
}

void CClub::addIntegration(int32_t nIntegration) {
	if (nIntegration < 0 && abs(nIntegration) > m_stBaseData.nIntegration) {
		m_stBaseData.nIntegration = 0;
	}
	else {
		m_stBaseData.nIntegration += nIntegration;
	}
	m_bMoneyDataDirty = true;
}

void CClub::addJoinedLeague(uint32_t nLeagueID) {
	m_stBaseData.vJoinedLeague.push_back(nLeagueID);
	m_bLeagueDataDirty = true;
}
void CClub::addCreatedLeague(uint32_t nLeagueID) {
	m_stBaseData.vCreatedLeague.push_back(nLeagueID);
	m_bLeagueDataDirty = true;
}

uint32_t CClub::getClubID() {
	return m_stBaseData.nClubID;
}

uint32_t CClub::getCreatorUID() {
	return m_stBaseData.nCreatorUID;
}

uint16_t CClub::getMemberLimit() {
	return m_stBaseData.nMemberLimit;
}

uint32_t CClub::getFoundation() {
	return m_stBaseData.nFoundation;
}

uint8_t CClub::getState() {
	return m_stBaseData.nState;
}

uint8_t CClub::getCreateRoomType() {
	return m_stBaseData.nCreateRoomType;
}

uint32_t CClub::getIntegration() {
	return m_stBaseData.nIntegration;
}

void CClub::insertIntoDB() {
	Json::Value jssql;
	char pBuffer[1024] = { 0 };
	sprintf_s(pBuffer, sizeof(pBuffer), "insert into club ( clubID,creator,name,headIcon,createTime,region,state,memberLimit,foundation,integration,description,createRoomType,searchLimit ) values ( %u,%u,'%s','%s',from_unixtime( %u ),'%s',%u,%u,%u,%u,'%s',%u,%u )", m_stBaseData.nClubID, m_stBaseData.nCreatorUID, m_stBaseData.cName, m_stBaseData.cHeadiconUrl, m_stBaseData.nCreateTime, m_stBaseData.cRegion, m_stBaseData.nState, m_stBaseData.nMemberLimit, m_stBaseData.nFoundation, m_stBaseData.nIntegration, m_stBaseData.cDescription, m_stBaseData.nCreateRoomType, m_stBaseData.nSearchLimit);
	jssql["sql"] = pBuffer;
	getClubMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB, m_stBaseData.nClubID, eAsync_DB_Add, jssql);

	getClubMemberData()->addMember(m_stBaseData.nCreatorUID, eClubMemberLevel_Creator);
}

bool CClub::dismissClub() {
	m_stBaseData.nState = eClubState_Delete;
	m_bLevelInfoDirty = true;
	return true;
}