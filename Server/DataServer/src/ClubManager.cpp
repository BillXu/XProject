#include "ClubManager.h"
#include "log4z.h"
#include "ClubDefine.h"
#include "AsyncRequestQuene.h"
#include "DataServerApp.h"

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
	readClubFormDB(10);
}

void CClubManager::onExit()
{
	auto iter = m_vAllClubs.begin();
	for (; iter != m_vAllClubs.end(); ++iter)
	{
		iter->second->onRelease();
	}
	IGlobalModule::onExit();
}

bool CClubManager::onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID)
{
	auto pTargetClub = getClubByClubID(nTargetID);
	if (pTargetClub && pTargetClub->onMsg(prealMsg, nMsgType, eSenderPort, nSenderID))
	{
		return true;
	}
	else
	{
		if (pTargetClub == NULL)
		{
			LOGFMTE("can not find session id = %d to process msg id = %d ,from = %d", nSenderID, nMsgType, eSenderPort);
		}
		else
		{
			LOGFMTE("unprocess msg for player uid = %d , msg = %d ,from %d ", nTargetID, nMsgType, eSenderPort);
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
	//default:
	//{
	//	if (jsReqContent["playerUID"].isNull() == false)
	//	{
	//		auto nPlayerUID = jsReqContent["playerUID"].asUInt();
	//		auto pPlayer = getPlayerByUserUID(nPlayerUID);
	//		if (pPlayer == nullptr)
	//		{
	//			jsResult["ret"] = 1; // can not find target player ;
	//			return false;
	//		}
	//		else
	//		{
	//			return pPlayer->onAsyncRequest(nRequestType, jsReqContent, jsResult);
	//		}
	//	}
	//}

	}
	return true;
}

bool CClubManager::onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID, uint16_t nTargetID) {
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

void CClubManager::readClubFormDB(uint8_t nOffset) {
	std::ostringstream ss;
	ss << "SELECT * FROM club where state !=  " << eClubState_Delete << " order by clubID desc limit 10 offset " << nOffset << ";";
	Json::Value jsReq;
	jsReq["sql"] = ss.str();
	getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB, rand(), eAsync_DB_Select, jsReq, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut) {
		if (isTimeOut)
		{
			LOGFMTE("caution: load club info from DB time out , caution!!!!!");
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
			auto nClubID = jsRow["clubID"].asUInt();
			auto iter = m_vAllClubs.find(nClubID);
			if (iter != m_vAllClubs.end())
			{
				LOGFMTE("bug: why already have this club id = %u , uid = %u double read , bug!!!!!!", nClubID);
				doProcessAfterReadDB();
				return;
			}

			auto pClub = new CClub();
			pClub->init();
			pClub->setClubID(nClubID);
			pClub->setCreatorUID(jsRow["creator"].asUInt());
			pClub->setName(jsRow["name"].asCString());
			pClub->setIcon(jsRow["headIcon"].asCString());
			pClub

			pMail->nMailType = (eMailType)jsRow["mailType"].asUInt();
			pMail->nState = (eMailState)jsRow["state"].asUInt();
			pMail->nPostTime = jsRow["postTime"].asUInt();

			m_vMails[pMail->nMailID] = pMail;
		}

		if (nAft < 3)  // only read one page ;
		{
			doProcessMailAfterReadDB();
			return;
		}

		// not finish , go on read 
		auto nSize = m_vMails.size();
		if (nSize >= 10)
		{
			doProcessMailAfterReadDB();
			return;
		}

		readMail(nSize);;

	});
}