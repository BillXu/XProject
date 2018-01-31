#include "League.h"
#include "LeagueMemberData.h"
#include "LeagueEvent.h"
#include "LeagueManager.h"
#include "LeagueGameData.h"
#include "ISeverApp.h"
#include "AsyncRequestQuene.h"
#include "log4z.h"
CLeague::CLeague() {
	memset(&m_stBaseData, 0, sizeof(m_stBaseData));
	m_bBaseDataDirty = false;
	m_bUseFulDataDirty = false;
	for (int i = eClubComponent_None; i < eClubComponent_Max; ++i)
	{
		m_vAllComponents[i] = NULL;
	}
}

CLeague::~CLeague() {
	for (int i = eLeagueComponent_None; i < eLeagueComponent_Max; ++i)
	{
		auto p = m_vAllComponents[i];
		if (p)
			delete p;
		m_vAllComponents[i] = NULL;
	}
}

void CLeague::init(CLeagueManager* pLeagueManager) {
	m_pLeagueManager = pLeagueManager;
	m_vAllComponents[eLeagueComponent_MemberData] = new CLeagueMemberData();
	m_vAllComponents[eLeagueComponent_GameData] = new CLeagueGameData();
	m_vAllComponents[eLeagueComponent_Event] = new CLeagueEvent();
	for (int i = eLeagueComponent_None; i < eLeagueComponent_Max; ++i)
	{
		auto p = m_vAllComponents[i];
		if (p)
		{
			p->init(this);
		}
	}
}

void CLeague::reset() {
	for (int i = eLeagueComponent_None; i < eLeagueComponent_Max; ++i)
	{
		auto p = m_vAllComponents[i];
		if (p)
		{
			p->reset();
		}
	}
	m_stBaseData.reset();
}

bool CLeague::onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort, uint32_t nSenderID) {
	if (MSG_LEAGUE_UPDATE_JOIN_LIMIT == nmsgType) {
		if (recvValue["clubID"].isNull() || recvValue["clubID"].isUInt() == false) {
			LOGFMTE("can not find clubID to update search limit league id = %u from session id = %u", getLeagueID(), nSenderID);
			Json::Value jsMsg;
			jsMsg["ret"] = 1;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			return true;
		}
		auto nClubID = recvValue["clubID"].asUInt();

		if (recvValue["uid"].isNull() || recvValue["uid"].isUInt() == false) {
			LOGFMTE("can not find uid to update search limit league id = %u from club id = %u", getLeagueID(), nClubID);
			Json::Value jsMsg;
			jsMsg["ret"] = 2;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			return true;
		}
		auto nUserID = recvValue["uid"].asUInt();

		if (recvValue["state"].isNull() || recvValue["state"].isUInt() == false) {
			LOGFMTE("can not find state to update search limit league id = %u from club id = %u", getLeagueID(), nClubID);
			Json::Value jsMsg;
			jsMsg["ret"] = 3;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			return true;
		}
		uint8_t nState = recvValue["state"].asUInt();
		if (nState > eLeagueJoinLimit_All) {
			LOGFMTE("state is illegal to update search limit league id = %u from club id = %u", getLeagueID(), nClubID);
			Json::Value jsMsg;
			jsMsg["ret"] = 4;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			return true;
		}

		if (getLeagueMemberData()->checkUpdateLevel(nClubID, eLeagueMemberLevel_Creator) == false) {
			LOGFMTE("club level is illegal to update search limit league id = %u from club id = %u", getLeagueID(), nClubID);
			Json::Value jsMsg;
			jsMsg["ret"] = 5;
			sendMsgToClient(jsMsg, nmsgType, nSenderID);
			return true;
		}

		setJoinLimit(nState);
		m_bUseFulDataDirty = true;
		Json::Value jsMsg;
		jsMsg["ret"] = 0;
		jsMsg["leagueID"] = getLeagueID();
		jsMsg["state"] = nState;
		sendMsgToClient(jsMsg, nmsgType, nSenderID);
		return true;
	}

	if (MSG_LEAGUE_APPLY_LEAGUE_INFO == nmsgType) {
		Json::Value jsMsg;
		jsMsg["leagueID"] = m_stBaseData.nLeagueID;
		jsMsg["name"] = m_stBaseData.cName;
		jsMsg["creator"] = m_stBaseData.nCreatorCID;
		jsMsg["ret"] = 0;
		sendMsgToClient(jsMsg, nmsgType, nSenderID);
		return true;
	}

	if (MSG_LEAGUE_APPLY_LEAGUE_DETAIL == nmsgType) {
		Json::Value jsMsg, jsMembers, jsJoinEvents;
		jsMsg["leagueID"] = m_stBaseData.nLeagueID;
		jsMsg["name"] = m_stBaseData.cName;
		jsMsg["creator"] = m_stBaseData.nCreatorCID;
		getLeagueMemberData()->memberDataToJson(jsMembers);
		jsMsg["members"] = jsMembers;
		jsMsg["joinLimit"] = m_stBaseData.nJoinLimit;
		getLeagueEvent()->joinEventWaitToJson(jsJoinEvents);
		jsMsg["joinEvents"] = jsJoinEvents;
		jsMsg["ret"] = 0;
		sendMsgToClient(jsMsg, nmsgType, nSenderID);
		return true;
	}

	for (int i = eLeagueComponent_None; i < eLeagueComponent_Max; ++i)
	{
		ILeagueComponent* p = m_vAllComponents[i];
		if (p)
		{
			if (p->onMsg(recvValue, nmsgType, eSenderPort, nSenderID))
			{
				return true;
			}
		}
	}

	return false;
}

bool CLeague::onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)
{
	for (int i = eLeagueComponent_None; i < eLeagueComponent_Max; ++i)
	{
		ILeagueComponent* p = m_vAllComponents[i];
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

ILeagueComponent* CLeague::getComponent(eLeagueComponentType eType) {
	if (eType < eLeagueComponent_Max) {
		return m_vAllComponents[eType];
	}
	return nullptr;
}

void CLeague::onTimerSave() {
	if (m_pLeagueManager->isReadingDB()) {
		return;
	}

	if (m_bBaseDataDirty) {
		m_bBaseDataDirty = false;
		Json::Value jssql;
		char pBuffer[512] = { 0 };
		sprintf_s(pBuffer, "update league set name = '%s', headIcon = '%s', state = %u where leagueID = %u;", m_stBaseData.cName, m_stBaseData.cHeadiconUrl, m_stBaseData.nState, m_stBaseData.nLeagueID);
		std::string str = pBuffer;
		jssql["sql"] = pBuffer;
		auto pReqQueue = m_pLeagueManager->getSvrApp()->getAsynReqQueue();
		pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, m_stBaseData.nLeagueID, eAsync_DB_Update, jssql);
	}

	if (m_bUseFulDataDirty) {
		m_bUseFulDataDirty = false;
		Json::Value jssql;
		char pBuffer[512] = { 0 };
		sprintf_s(pBuffer, "update league set memberLimit = %u, joinLimit = %u where leagueID = %u;", m_stBaseData.nMemberLimit, m_stBaseData.nJoinLimit, m_stBaseData.nLeagueID);
		std::string str = pBuffer;
		jssql["sql"] = pBuffer;
		auto pReqQueue = m_pLeagueManager->getSvrApp()->getAsynReqQueue();
		pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, m_stBaseData.nLeagueID, eAsync_DB_Update, jssql);
	}

	for (int i = eLeagueComponent_None; i < eLeagueComponent_Max; ++i)
	{
		auto p = m_vAllComponents[i];
		if (p)
		{
			p->timerSave();
		}
	}
}

void CLeague::sendMsgToClient(Json::Value& jsMsg, uint16_t nMsgType, uint32_t nSessionID) {
	getLeagueMgr()->sendMsg(jsMsg, nMsgType, getLeagueID(), nSessionID);
}

void CLeague::setLeagueID(uint32_t nLeagueID) {
	m_stBaseData.nLeagueID = nLeagueID;
}

void CLeague::setCreatorCID(uint32_t nCreatorCID) {
	m_stBaseData.nCreatorCID = nCreatorCID;
}

void CLeague::setName(const char* cName) {
	sprintf_s(m_stBaseData.cName, "%s", cName);
}

void CLeague::setIcon(const char* cIcon) {
	sprintf_s(m_stBaseData.cHeadiconUrl, "%s", cIcon);
}

void CLeague::setCreateTime(uint32_t nTime) {
	m_stBaseData.nCreateTime = nTime;
}

void CLeague::setState(uint8_t nState) {
	m_stBaseData.nState = nState;
}

void CLeague::setMemberLimit(uint16_t nMemberLimit) {
	m_stBaseData.nMemberLimit = nMemberLimit;
}

void CLeague::setJoinLimit(uint8_t nType) {
	m_stBaseData.nJoinLimit = nType;
}

uint32_t CLeague::getLeagueID() {
	return m_stBaseData.nLeagueID;
}

uint32_t CLeague::getCreatorCID() {
	return m_stBaseData.nCreatorCID;
}

uint16_t CLeague::getMemberLimit() {
	return m_stBaseData.nMemberLimit;
}
uint8_t CLeague::getJoinLimit() {
	return m_stBaseData.nJoinLimit;
}

uint8_t CLeague::getState() {
	return m_stBaseData.nState;
}

void CLeague::insertIntoDB() {
	Json::Value jssql;
	char pBuffer[1024] = { 0 };
	sprintf_s(pBuffer, sizeof(pBuffer), "insert into league ( leagueID,creator,name,headIcon,createTime,state,memberLimit,joinLimit ) values ( %u,%u,'%s','%s',from_unixtime( %u ),%u,%u,%u )", m_stBaseData.nLeagueID, m_stBaseData.nCreatorCID, m_stBaseData.cName, m_stBaseData.cHeadiconUrl, m_stBaseData.nCreateTime, m_stBaseData.nState, m_stBaseData.nMemberLimit, m_stBaseData.nJoinLimit);
	jssql["sql"] = pBuffer;
	getLeagueMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB, m_stBaseData.nLeagueID, eAsync_DB_Add, jssql);

	getLeagueMemberData()->addMember(m_stBaseData.nCreatorCID, eLeagueMemberLevel_Creator);
}

bool CLeague::dismissLeague(uint32_t nClubID) {
	auto nLevel = getLeagueMemberData()->getMemberLevel(nClubID);
	if (nLevel && nLevel == eLeagueMemberLevel_Creator) {
		m_stBaseData.nState = eClubState_Delete;
		m_bBaseDataDirty = true;
		return true;
	}
	return false;
}