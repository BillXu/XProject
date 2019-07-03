#include "PlayerBaseData.h"
#include <string>
#include "ServerMessageDefine.h"
#include "Player.h"
#include "log4z.h"
#include <time.h>
#include "DataServerApp.h"
#include "PlayerEvent.h"
#include "PlayerManager.h"
#include "EventCenter.h"
#include "AutoBuffer.h"
#include "AsyncRequestQuene.h"
#include <assert.h>
#include "PlayerGameData.h"
#include "Club.h"
#pragma warning( disable : 4996 )
CPlayerBaseData::CPlayerBaseData(CPlayer* player )
	:IPlayerComponent(player)
{
	m_stBaseData.reset();
	m_eType = ePlayerComponent_BaseData ;
	m_isReadingDB = false;
	m_bPlayerInfoDirty = false;
	m_nTmpCoin = 0;
	m_nTmpDiamond = 0;
}

CPlayerBaseData::~CPlayerBaseData()
{

}

void CPlayerBaseData::init()
{
	m_stBaseData.reset();
	m_vCreatedClubIDs.clear();
	m_stBaseData.nUserUID = getPlayer()->getUserUID() ;
	m_isReadingDB = false;
	m_bPlayerInfoDirty = false;
	m_bMoneyDataDirty = false;
	m_bPointDataDirty = false;
	m_bVipDataDirty = false;
	m_nTmpCoin = 0;
	m_nTmpDiamond = 0;
}

void CPlayerBaseData::reset()
{
	m_stBaseData.reset();
	m_vCreatedClubIDs.clear();
	m_bMoneyDataDirty = false;
	m_isReadingDB = false;
	m_bPlayerInfoDirty = false;
	m_bPointDataDirty = false;
	m_bVipDataDirty = false;
	m_nTmpCoin = 0;
	m_nTmpDiamond = 0;
}

void CPlayerBaseData::onPlayerLogined()
{
	if ( m_isReadingDB )
	{
		LOGFMTE("player uid = %u already reading from db , why try again ? ",getPlayer()->getUserUID() );
		return;
	}

	m_stBaseData.nUserUID = getPlayer()->getUserUID();
	Json::Value jssql;
	char pBuffer[512] = { 0 };
	sprintf_s(pBuffer, "SELECT nickName,sex,coin,diamond,emojiCnt,headIcon,clubs,takeCharityTimes,lastTakeCardGiftTime,totalDiamond,totalGame,gateLevel,totalPoint,point,withdrawPoint,pointCalculateData,pointTotalGame,withdrawTotalGame,vipLevel,vipInvalidTime FROM playerbasedata where userUID = %u ;",getPlayer()->getUserUID());
	std::string str = pBuffer;
	jssql["sql"] = pBuffer;
	auto pReqQueue = getPlayer()->getPlayerMgr()->getSvrApp()->getAsynReqQueue();
	pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, getPlayer()->getUserUID(),eAsync_DB_Select, jssql, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut )
	{
		if (isTimeOut)
		{
			return;
		}
		Json::Value jsData = retContent["data"];
		if (jsData.size() == 0)
		{
			LOGFMTE("why read player uid = %u base data is null ? ",getPlayer()->getUserUID() );
			return;
		}
		m_isReadingDB = false;

		Json::Value jsRow = jsData[0u];
		sprintf_s(m_stBaseData.cHeadiconUrl,"%s", jsRow["headIcon"].asCString() );
		sprintf_s(m_stBaseData.cName, "%s", jsRow["nickName"].asCString());
		m_stBaseData.nCoin = jsRow["coin"].asUInt();
		m_stBaseData.nDiamoned = jsRow["diamond"].asUInt();
		m_stBaseData.nEmojiCnt = jsRow["emojiCnt"].asUInt();
		m_stBaseData.nSex = jsRow["sex"].asUInt();
		m_stBaseData.nTakeCharityTimes = jsRow["takeCharityTimes"].asUInt();
		m_stBaseData.tLastTakeCardGiftTime = jsRow["lastTakeCardGiftTime"].asUInt();
		m_stBaseData.nTotalDiamond = jsRow["totalDiamond"].asUInt();
		m_stBaseData.nTotalGame = jsRow["totalGame"].asUInt();
		m_stBaseData.nGateLevel = jsRow["gateLevel"].asUInt();
		m_stBaseData.nTotalPoint = jsRow["totalPoint"].asUInt();
		m_stBaseData.nPoint = jsRow["point"].asUInt();
		m_stBaseData.nWithdrawPoint = jsRow["withdrawPoint"].asUInt();
		m_stBaseData.tPointCalculateData = jsRow["pointCalculateData"].asUInt();
		m_stBaseData.nPointTotalGame = jsRow["pointTotalGame"].asUInt();
		m_stBaseData.nWithdrawTotalGame = jsRow["withdrawTotalGame"].asUInt();
		m_stBaseData.nVipLevel = jsRow["vipLevel"].asUInt();
		m_stBaseData.tVipInvalidTime = jsRow["vipInvalidTime"].asUInt();

		Json::Value jsClubs;
		Json::Reader jsR;
		if (jsR.parse(jsRow["clubs"].asString(), jsClubs))
		{
			for (uint16_t nIdx = 0; nIdx < jsClubs.size(); ++nIdx)
			{
				m_stBaseData.vJoinedClubIDs.push_back(jsClubs[nIdx].asUInt());
			}
		}

		modifyMoney(m_nTmpCoin);
		modifyMoney(m_nTmpDiamond,true);
		m_nTmpCoin = 0;
		m_nTmpDiamond = 0;
		sortVipInfo();
		sendBaseDataToClient();
	}
	);

	m_isReadingDB = true;
}

void CPlayerBaseData::onPlayerOtherDeviceLogin(uint32_t nOldSessionID, uint32_t nNewSessionID)
{
	sendBaseDataToClient();
}

bool CPlayerBaseData::onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort )
{
	if ( MSG_PLAYER_UPDATE_INFO == nmsgType )
	{
		sprintf_s(m_stBaseData.cHeadiconUrl, "%s", recvValue["headIcon"].asCString());
		sprintf_s(m_stBaseData.cName, "%s", recvValue["name"].asCString());
		m_stBaseData.nSex = recvValue["sex"].asUInt();
		m_bPlayerInfoDirty = true;
		return true;
	}
	else if ( MSG_PLAYER_UPDATE_GPS == nmsgType )
	{
		m_stBaseData.dfJ = recvValue["J"].asDouble();
		m_stBaseData.dfW = recvValue["W"].asDouble();
		m_stBaseData.sAddress = recvValue["address"].asString();
		return true;
	}
	else if ( MSG_PLAYER_REFRESH_MONEY == nmsgType )
	{
		Json::Value jsmsg;
		jsmsg["coin"] = getCoin();
		jsmsg["diamond"] = getDiamoned();
		jsmsg["emojiCnt"] = getEmojiCnt();
		sendMsg(jsmsg, nmsgType);
		return true;
	}
	else if ( MSG_REQUEST_JOINED_CLUBS == nmsgType )
	{
		Json::Value jsmsg;
		Json::Value jsArrayClub;
		for (auto& ref : m_stBaseData.vJoinedClubIDs)
		{
			jsArrayClub[jsArrayClub.size()] = ref;
		}
		jsmsg["clubs"] = jsArrayClub;
		sendMsg(jsmsg, nmsgType);
		return true;
	}
	else if (MSG_GET_SHARE_PRIZE == nmsgType) {
		uint8_t shareTimes = m_stBaseData.nTakeCharityTimes;
		uint8_t allSharedTimes = shareTimes >> 1;
		uint8_t sharedTimeLimit = 0;

		if (sharedTimeLimit && allSharedTimes > sharedTimeLimit) {
			recvValue["diamond"] = 0;
			recvValue["sharetimes"] = shareTimes;
			sendMsg(recvValue, nmsgType);
			return true;
		}

		uint8_t curFlag = shareTimes & 0x1;
		// check times limit state ;
		time_t tNow = time(nullptr);
		struct tm pTimeCur;
		struct tm pTimeLast;
		pTimeCur = *localtime(&tNow);
		time_t nLastTakeTime = m_stBaseData.tLastTakeCardGiftTime;
		pTimeLast = *localtime(&nLastTakeTime);
		if (pTimeCur.tm_year == pTimeLast.tm_year && pTimeCur.tm_yday == pTimeLast.tm_yday) // the same day ; do nothing
		{

		}
		else
		{
			curFlag = 0; // new day reset times ;
		}

		if (curFlag >= 1)
		{
			recvValue["diamond"] = 0;
			recvValue["sharetimes"] = shareTimes;
			sendMsg(recvValue, nmsgType);
			return true;
		}

		m_bPlayerInfoDirty = true;
		allSharedTimes++;
		curFlag = 1;
		m_stBaseData.nTakeCharityTimes = (allSharedTimes << 1) | curFlag;
		//++m_stBaseData.nTakeCharityTimes;
		m_stBaseData.tLastTakeCardGiftTime = time(NULL);
		uint32_t nGiveDiamond = 1;
		modifyMoney(nGiveDiamond, true);
		recvValue["diamond"] = nGiveDiamond;
		recvValue["sharetimes"] = m_stBaseData.nTakeCharityTimes;
		sendMsg(recvValue, nmsgType);
		LOGFMTD("player = %u gets shared prize = %u now has got = %u times", getPlayer()->getUserUID(), nGiveDiamond, allSharedTimes);
		return true;
	}
	else if (MSG_PLAYER_GET_POINT_INFO == nmsgType) {
		calculatePointWithdraw();
		sendPointInfo();
		return true;
	}
	else if (MSG_PLAYER_WITHDRAW_POINT == nmsgType) {
		withdarwPoint();
		return true;
	}

	return false ;
}

void CPlayerBaseData::sendBaseDataToClient()
{
	if ( m_isReadingDB )
	{
		LOGFMTE( "player is reading from db can not send to client  , uid = %u",getPlayer()->getUserUID() );
		return;
	 }

	if (getPlayer()->getSessionID() == 0) {
		LOGFMTE("player is not real online can not send to client  , uid = %u", getPlayer()->getUserUID());
		return;
	}

	Json::Value jsBaseData;
	jsBaseData["uid"] = getPlayer()->getUserUID();
	jsBaseData["name"] = getPlayerName();
	jsBaseData["sex"] = getSex();
	jsBaseData["headIcon"] = getHeadIcon();
	jsBaseData["diamond"] = getDiamoned();
	jsBaseData["coin"] = getCoin();
	jsBaseData["emojiCnt"] = getEmojiCnt();
	jsBaseData["ip"] = getPlayer()->getIp();
	jsBaseData["takeCharityTimes"] = m_stBaseData.nTakeCharityTimes;
	jsBaseData["lastTakeCardGiftTime"] = m_stBaseData.tLastTakeCardGiftTime;
	jsBaseData["point"] = getPoint();
	jsBaseData["vipLevel"] = getVipLevel();
	jsBaseData["vipInvalidTime"] = m_stBaseData.tVipInvalidTime;

	Json::Value jsArrayClub;
	for (auto& ref : m_stBaseData.vJoinedClubIDs)
	{
		jsArrayClub[jsArrayClub.size()] = ref;
	}
	jsBaseData["clubs"] = jsArrayClub;

	auto pStay = ((CPlayerGameData*)getPlayer()->getComponent(ePlayerComponent_PlayerGameData))->getStayInRoom();
	if (pStay.isEmpty() == false)
	{
		jsBaseData["stayRoomID"] = pStay.nRoomID;
	}
	sendMsg(jsBaseData, MSG_PLAYER_BASE_DATA);
	SendGateIP();
	LOGFMTD("send msg to client base data uid = %u", getPlayer()->getUserUID() );
}

void CPlayerBaseData::timerSave()
{
	if ( m_isReadingDB == true )
	{
		return;
	}

	saveMoney();
	savePoint();
	saveVipInfo();
	// check player info ;
	if ( m_bPlayerInfoDirty )
	{
		m_bPlayerInfoDirty = false;

		Json::Value jsArrayClub;
		for (auto& ref : m_stBaseData.vJoinedClubIDs)
		{
			jsArrayClub[jsArrayClub.size()] = ref;
		}
		Json::StyledWriter jsw;
		auto strClubs = jsw.write(jsArrayClub);

		Json::Value jssql;
		char pBuffer[1024] = { 0 };
		sprintf_s(pBuffer, "update playerbasedata set nickName = '%s' ,sex = %u , headIcon = '%s',clubs = '%s',takeCharityTimes = %u,lastTakeCardGiftTime = %u,totalDiamond = %u,totalGame = %u,gateLevel = %u where userUID = %u ;", getPlayerName(),getSex(),getHeadIcon(), strClubs.c_str(), m_stBaseData.nTakeCharityTimes, m_stBaseData.tLastTakeCardGiftTime, m_stBaseData.nTotalDiamond, m_stBaseData.nTotalGame, m_stBaseData.nGateLevel, getPlayer()->getUserUID());
		std::string str = pBuffer;
		jssql["sql"] = pBuffer;
		auto pReqQueue = getPlayer()->getPlayerMgr()->getSvrApp()->getAsynReqQueue();
		pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, getPlayer()->getUserUID(),  eAsync_DB_Update, jssql);
	}
}

void CPlayerBaseData::saveMoney()
{
	if ( m_bMoneyDataDirty == false )
	{
		return;
	}
	m_bMoneyDataDirty = false;

	Json::Value jssql;
	char pBuffer[512] = { 0 };
	sprintf_s(pBuffer, "update playerbasedata set coin = %u ,diamond = %u,emojiCnt = %u where userUID = %u ;", getCoin(),getDiamoned(),getEmojiCnt(), getPlayer()->getUserUID());
	std::string str = pBuffer;
	jssql["sql"] = pBuffer;
	auto pReqQueue = getPlayer()->getPlayerMgr()->getSvrApp()->getAsynReqQueue();
	pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, getPlayer()->getUserUID(), eAsync_DB_Update, jssql);
	LOGFMTD( "uid = %u save money coin = %u , diamond = %u , emojiCnt = %u",getPlayer()->getUserUID(),getCoin(),getDiamoned(), getEmojiCnt() );
}

bool CPlayerBaseData::modifyMoney( int32_t nOffset, bool bDiamond )
{
	if ( m_isReadingDB )
	{
		int32_t& nRefTmp = bDiamond ? m_nTmpDiamond : m_nTmpCoin;
		nRefTmp += nOffset;
		return true;
	}

	uint32_t& nRefMoney = bDiamond ? m_stBaseData.nDiamoned : m_stBaseData.nCoin;
	if ( nOffset < 0 )
	{
		if ((uint32_t)(nOffset * -1) > nRefMoney)
		{
			LOGFMTE("uid = %u money is too few , can not decrease coin = %u , diamond = %u , offset = %d", getPlayer()->getUserUID(), getCoin(), getDiamoned(), nOffset);
			nOffset = (-1 * (int32_t)nRefMoney);
		}
	}

	nRefMoney += nOffset;
	m_bMoneyDataDirty = true;

	if (bDiamond && nOffset > 0) {
		for (auto& ref : m_vCreatedClubIDs) {
			auto pClub = DataServerApp::getInstance()->getClubMgr()->getClub(ref);
			if (pClub && pClub->getCreatorUID() == m_stBaseData.nUserUID) {
				pClub->clearLackDiamond();
			}
		}
	}
	return true;
}

bool CPlayerBaseData::modifyEmojiCnt(int32_t nOffset)
{
	if (m_isReadingDB)
	{
		LOGFMTE( "can not modify emojicnt reading db uid = %u , offset = %d",getPlayer()->getUserUID(),nOffset );
		return false;
	}

	if (nOffset < 0)
	{
		if ((uint32_t)(nOffset * -1) > getEmojiCnt() )
		{
			LOGFMTE("uid = %u emoji cnt is too few , can not decrease emojCnt = %u ,offset = %d", getPlayer()->getUserUID(), getEmojiCnt(),  nOffset);
			nOffset = (-1 * (int32_t)getEmojiCnt());
		}
	}

	m_stBaseData.nEmojiCnt += nOffset;
	m_bMoneyDataDirty = true;
	return true;
}

void CPlayerBaseData::onLeaveClub(uint32_t nClubID)
{
	auto iter = std::find(m_stBaseData.vJoinedClubIDs.begin(), m_stBaseData.vJoinedClubIDs.end(),nClubID);
	if ( iter != m_stBaseData.vJoinedClubIDs.end() )
	{
		m_stBaseData.vJoinedClubIDs.erase(iter);
		m_bPlayerInfoDirty = true;
	}
	else
	{
		LOGFMTE("player not in club = %u , uid = %u, how to leave", nClubID, m_stBaseData.nUserUID);
	}

	iter = std::find(m_vCreatedClubIDs.begin(), m_vCreatedClubIDs.end(), nClubID);
	if (iter != m_vCreatedClubIDs.end())
	{
		LOGFMTE("player do dismiss club = %u , uid = %u", nClubID, m_stBaseData.nUserUID);
		m_vCreatedClubIDs.erase(iter);
	}
}

void CPlayerBaseData::onJoinClub(uint32_t nClubID)
{
	auto iter = std::find(m_stBaseData.vJoinedClubIDs.begin(), m_stBaseData.vJoinedClubIDs.end(), nClubID);
	if (iter != m_stBaseData.vJoinedClubIDs.end())
	{
		LOGFMTE( "why add twice club = %u , uid = %u",nClubID,m_stBaseData.nUserUID );
		return;
	}
	m_stBaseData.vJoinedClubIDs.push_back(nClubID);
	m_bPlayerInfoDirty = true;
}

void CPlayerBaseData::onCreatedClub(uint32_t nClubID) {
	auto iter = std::find(m_vCreatedClubIDs.begin(), m_vCreatedClubIDs.end(), nClubID);
	if (iter != m_vCreatedClubIDs.end())
	{
		LOGFMTE("why create twice club = %u , uid = %u", nClubID, m_stBaseData.nUserUID);
		return;
	}
	m_vCreatedClubIDs.push_back(nClubID);
}

void CPlayerBaseData::eraseCreatedClub(uint32_t nClubID) {
	auto iter = std::find(m_vCreatedClubIDs.begin(), m_vCreatedClubIDs.end(), nClubID);
	if (iter != m_vCreatedClubIDs.end())
	{
		LOGFMTE("player do transfer club = %u , uid = %u", nClubID, m_stBaseData.nUserUID);
		m_vCreatedClubIDs.erase(iter);
	}
}

bool CPlayerBaseData::canRemovePlayer() {
	if (m_vCreatedClubIDs.size()) {
		return false;
	}
	return true;
}

void CPlayerBaseData::SendGateIP() {
	if (getGateLevel()) {
		if (((m_stBaseData.nTotalDiamond == 0 && m_stBaseData.nTotalGame < 16) ||
			(m_stBaseData.nTotalDiamond < 200 && m_stBaseData.nTotalGame < 8)) &&
			(getGateLevel() == 1 || getGateLevel() == 2)) {
			auto sGateIP = getPlayer()->getPlayerMgr()->getSpecialGateIP(getPlayer()->getUserUID());
			if (sGateIP.empty() == false) {
				Json::Value jsMsg;
				jsMsg["IP"] = sGateIP.c_str();
				getPlayer()->sendMsgToClient(jsMsg, MSG_PLAYER_REFRESH_GATE_IP);
			}
		}
		else {
			auto sGateIP = getPlayer()->getPlayerMgr()->getGateIP(getGateLevel(), getPlayer()->getUserUID());
			if (sGateIP.empty() == false) {
				Json::Value jsMsg;
				jsMsg["IP"] = sGateIP.c_str();
				getPlayer()->sendMsgToClient(jsMsg, MSG_PLAYER_REFRESH_GATE_IP);
			}
		}
	}
}

uint8_t CPlayerBaseData::getGateLevel() {
	return m_stBaseData.nGateLevel;
}

void CPlayerBaseData::setGateLevel(uint8_t nGateLevel) {
	m_stBaseData.nGateLevel = nGateLevel;
	SendGateIP();
	m_bPlayerInfoDirty = true;
}

void CPlayerBaseData::addGameCnt() {
	m_stBaseData.nTotalGame++;

	if (m_stBaseData.nTotalGame == 8) {
		//m_stBaseData.nGateLevel = 1;
		if (getGateLevel() < 1) {
			setGateLevel(1);
		}
	}
	else if (m_stBaseData.nTotalGame == 40) {
		//m_stBaseData.nGateLevel = 2;
		if (getGateLevel() < 2) {
			setGateLevel(2);
		}
	}
	else if (m_stBaseData.nTotalGame == 80) {
		//m_stBaseData.nGateLevel = 3;
		if (getGateLevel() < 3) {
			setGateLevel(3);
		}
	}

	m_bPlayerInfoDirty = true;
}

void CPlayerBaseData::addTotalDiamond(int32_t nDiamond) {
	if (nDiamond < 0) {
		if (-1 * nDiamond > m_stBaseData.nTotalDiamond) {
			m_stBaseData.nTotalDiamond = 0;
		}
		else {
			m_stBaseData.nTotalDiamond += nDiamond;
		}
	}
	else {
		m_stBaseData.nTotalDiamond += nDiamond;
	}

	if (m_stBaseData.nTotalDiamond > 999) {
		//m_stBaseData.nGateLevel = 3;
		if (getGateLevel() < 3) {
			setGateLevel(3);
		}
	}
	else if (m_stBaseData.nTotalDiamond > 99) {
		//m_stBaseData.nGateLevel = 2;
		if (getGateLevel() < 2) {
			setGateLevel(2);
		}
	}
	else if (m_stBaseData.nTotalDiamond > 9) {
		//m_stBaseData.nGateLevel = 1;
		if (getGateLevel() < 1) {
			setGateLevel(1);
		}
	}

	m_bPlayerInfoDirty = true;
}

uint32_t CPlayerBaseData::getPoint() {
	return m_stBaseData.nPoint;
}

uint32_t CPlayerBaseData::getTotalPoint() {
	return m_stBaseData.nTotalPoint;
}

uint32_t CPlayerBaseData::getWithdrawPoint() {
	return m_stBaseData.nWithdrawPoint;
}

void CPlayerBaseData::savePoint() {
	if (m_bPointDataDirty == false)
	{
		return;
	}
	m_bPointDataDirty = false;

	Json::Value jssql;
	char pBuffer[512] = { 0 };
	sprintf_s(pBuffer, "update playerbasedata set totalPoint = %u, point = %u, withdrawPoint = %u, pointCalculateData = %u, pointTotalGame = %u, withdrawTotalGame = %u where userUID = %u;", m_stBaseData.nTotalPoint, m_stBaseData.nPoint, m_stBaseData.nWithdrawPoint, m_stBaseData.tPointCalculateData, m_stBaseData.nPointTotalGame, m_stBaseData.nWithdrawTotalGame, getPlayer()->getUserUID());
	std::string str = pBuffer;
	jssql["sql"] = pBuffer;
	auto pReqQueue = getPlayer()->getPlayerMgr()->getSvrApp()->getAsynReqQueue();
	pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, getPlayer()->getUserUID(), eAsync_DB_Update, jssql);
	LOGFMTD("uid = %u save totalPoint = %u, point = %u , withdrawPoint = %u , pointCalculateData = %u, pointTotalGame = %u, withdrawTotalGame = %u", getPlayer()->getUserUID(), m_stBaseData.nTotalPoint, m_stBaseData.nPoint, m_stBaseData.nWithdrawPoint, m_stBaseData.tPointCalculateData, m_stBaseData.nPointTotalGame, m_stBaseData.nWithdrawTotalGame);
}

void CPlayerBaseData::savePointRecord(int32_t nOffset, Json::Value jsDetail) {
	Json::Value jsSql;
	Json::StyledWriter jsWrite;
	std::string sDetail = jsWrite.write(jsDetail);
	char pBuffer[512] = { 0 };
	sprintf_s(pBuffer, sizeof(pBuffer), "call addPointRecord(%u,%d,%u,'%s');", getPlayer()->getUserUID(), nOffset, getPoint(), sDetail.c_str());
	std::string str = pBuffer;
	jsSql["sql"] = pBuffer;
	auto pReqQueue = getPlayer()->getPlayerMgr()->getSvrApp()->getAsynReqQueue();
	pReqQueue->pushAsyncRequest(ID_MSG_PORT_RECORDER_DB, getPlayer()->getUserUID(), eAsync_DB_Select, jsSql);
}

void CPlayerBaseData::addPointTotalGameCnt() {
	calculatePointWithdraw();
	m_stBaseData.nPointTotalGame++;
	LOGFMTI("player uid = %u add point game cnt = %u", m_stBaseData.nUserUID, m_stBaseData.nPointTotalGame);
	m_bPointDataDirty = true;
}

bool CPlayerBaseData::addPoint(int32_t nOffset) {
	if (m_isReadingDB) {
		LOGFMTE("player uid = %u add point error offset = %d point = %u, player is reading DB?", m_stBaseData.nUserUID, nOffset, m_stBaseData.nPoint);
		return false;
	}

	if (nOffset < 0) {
		uint32_t nAbsOffset = abs(nOffset);
		if (nAbsOffset > m_stBaseData.nPoint) {
			LOGFMTE("player uid = %u add point error offset = %d point = %u", m_stBaseData.nUserUID, nOffset, m_stBaseData.nPoint);
			return false;
			/*m_stBaseData.nPoint = 0;
			sendPointInfo();
			return true;*/
		}
	}
	m_bPointDataDirty = true;
	LOGFMTI("player uid = %u add point = %d", m_stBaseData.nUserUID, nOffset);
	m_stBaseData.nPoint += nOffset;
	sendPointInfo();
	return true;
}

void CPlayerBaseData::calculatePointWithdraw() {
	if (m_stBaseData.tPointCalculateData) {
		time_t tNow = time(nullptr);
		time_t tCal = m_stBaseData.tPointCalculateData;
		struct tm pTimeCur, pTimeCal;
		pTimeCur = *localtime(&tNow);
		pTimeCal = *localtime(&tCal);
		if (pTimeCur.tm_yday == pTimeCal.tm_yday) {
			return;
		}
		else if (pTimeCur.tm_yday == pTimeCal.tm_yday + 1) {
			sortPointWithDraw();
			return;
		}
	}

	m_stBaseData.nWithdrawPoint = 0;
	m_stBaseData.tPointCalculateData = time(nullptr);
	m_stBaseData.nPointTotalGame = 0;
	m_stBaseData.nWithdrawTotalGame = 0;
	m_bPointDataDirty = true;
}

void CPlayerBaseData::sortPointWithDraw() {
	//begin sort
	m_stBaseData.nWithdrawPoint = 0;
	m_stBaseData.tPointCalculateData = time(nullptr);

	// game cnt
	if (m_stBaseData.nPointTotalGame) {
		if (m_stBaseData.nPointTotalGame > 49) {
			m_stBaseData.nWithdrawPoint += 450;
		}
		else if (m_stBaseData.nPointTotalGame > 39) {
			m_stBaseData.nWithdrawPoint += 350;
		}
		else if (m_stBaseData.nPointTotalGame > 29) {
			m_stBaseData.nWithdrawPoint += 250;
		}
		else if (m_stBaseData.nPointTotalGame > 19) {
			m_stBaseData.nWithdrawPoint += 150;
		}
		else if (m_stBaseData.nPointTotalGame > 14) {
			m_stBaseData.nWithdrawPoint += 100;
		}
		else if (m_stBaseData.nPointTotalGame > 9) {
			m_stBaseData.nWithdrawPoint += 50;
		}
		else if (m_stBaseData.nPointTotalGame > 5) {
			m_stBaseData.nWithdrawPoint += 25;
		}
		else if (m_stBaseData.nPointTotalGame > 2) {
			m_stBaseData.nWithdrawPoint += 5;
		}
		else {
			m_stBaseData.nWithdrawPoint += 1;
		}
	}
	m_stBaseData.nWithdrawTotalGame = m_stBaseData.nPointTotalGame;
	m_stBaseData.nPointTotalGame = 0;

	//end sort
	m_bPointDataDirty = true;
}

void CPlayerBaseData::withdarwPoint() {
	calculatePointWithdraw();

	if (m_stBaseData.nWithdrawPoint) {
		auto nWithDrawCnt = m_stBaseData.nWithdrawPoint;
		m_stBaseData.nWithdrawPoint = 0;
		m_stBaseData.nTotalPoint += nWithDrawCnt;
		if (addPoint(nWithDrawCnt)) {
			Json::Value jsDetail;
			jsDetail["gameCnt"] = m_stBaseData.nWithdrawTotalGame;
			savePointRecord(nWithDrawCnt, jsDetail);
			CPlayerManager::PlayerTotalPointRankInfo ptpri;
			ptpri.nUserUID = getPlayer()->getUserUID();
			ptpri.nTotalPoint = m_stBaseData.nTotalPoint;
			getPlayer()->getPlayerMgr()->addPTPR(ptpri);
		}
		else {
			m_stBaseData.nWithdrawPoint = nWithDrawCnt;
			m_stBaseData.nTotalPoint -= nWithDrawCnt;
			LOGFMTE("ERROR! Player uid = %u withdraw point = %u error", m_stBaseData.nUserUID, nWithDrawCnt);
		}
	}
	else {
		sendPointInfo();
	}
}

bool CPlayerBaseData::addTotalPoint(int32_t nOffset) {
	if (m_isReadingDB) {
		LOGFMTE("player uid = %u add total point error offset = %d total point = %u, player is reading DB?", m_stBaseData.nUserUID, nOffset, m_stBaseData.nTotalPoint);
		return false;
	}

	if (nOffset < 0) {
		uint32_t nAbsOffset = abs(nOffset);
		if (nAbsOffset > m_stBaseData.nTotalPoint) {
			LOGFMTE("player uid = %u add total point error offset = %d total point = %u", m_stBaseData.nUserUID, nOffset, m_stBaseData.nTotalPoint);
			return false;
			/*m_stBaseData.nPoint = 0;
			sendPointInfo();
			return true;*/
		}
	}
	m_bPointDataDirty = true;
	LOGFMTI("player uid = %u add total point = %d", m_stBaseData.nUserUID, nOffset);
	m_stBaseData.nTotalPoint += nOffset;
	sendPointInfo();
	return true;
}

void CPlayerBaseData::sendPointInfo() {
	Json::Value jsMsg;
	jsMsg["totalPoint"] = m_stBaseData.nTotalPoint;
	jsMsg["point"] = m_stBaseData.nPoint;
	jsMsg["withdraw"] = m_stBaseData.nWithdrawPoint;
	jsMsg["totalGame"] = m_stBaseData.nPointTotalGame;
	jsMsg["withdrawTotalGame"] = m_stBaseData.nWithdrawTotalGame;
	getPlayer()->sendMsgToClient(jsMsg, MSG_PLAYER_GET_POINT_INFO);
}

uint32_t CPlayerBaseData::getVipLevel() {
	return m_stBaseData.nVipLevel;
}

uint32_t CPlayerBaseData::getVipInvalidTime() {
	return m_stBaseData.tVipInvalidTime;
}

void CPlayerBaseData::saveVipInfo() {
	sortVipInfo();

	if (m_bVipDataDirty == false)
	{
		return;
	}
	m_bVipDataDirty = false;

	Json::Value jssql;
	char pBuffer[512] = { 0 };
	sprintf_s(pBuffer, "update playerbasedata set vipLevel = %u ,vipInvalidTime = %u where userUID = %u ;", m_stBaseData.nVipLevel, m_stBaseData.tVipInvalidTime, getPlayer()->getUserUID());
	std::string str = pBuffer;
	jssql["sql"] = pBuffer;
	auto pReqQueue = getPlayer()->getPlayerMgr()->getSvrApp()->getAsynReqQueue();
	pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, getPlayer()->getUserUID(), eAsync_DB_Update, jssql);
	LOGFMTD("uid = %u save vipLevel = %u , vipInvalidTime = %u", getPlayer()->getUserUID(), m_stBaseData.nVipLevel, m_stBaseData.tVipInvalidTime);
}

void CPlayerBaseData::sortVipInfo() {
	if (getVipLevel()) {
		auto tNow = time(nullptr);
		if (m_stBaseData.tVipInvalidTime > tNow) {
			return;
		}

		m_stBaseData.nVipLevel = 0;
		m_bVipDataDirty = true;
		sendVipInfo();
	}
}

void CPlayerBaseData::sendVipInfo() {
	Json::Value jsMsg;
	jsMsg["vipLevel"] = getVipLevel();
	jsMsg["vipInvalidTime"] = m_stBaseData.tVipInvalidTime;
	getPlayer()->sendMsgToClient(jsMsg, MSG_PLAYER_GET_VIP_INFO);
}

uint8_t CPlayerBaseData::changeVip(uint32_t nVipLevel, uint32_t nDay) {
	sortVipInfo();
	LOGFMTD("uid = %u change vipLevel = %u , dayTime = %u", getPlayer()->getUserUID(), nVipLevel, nDay);
	uint8_t nRet = 0;
	if (nVipLevel) {
		if (nDay) {
			if (getVipLevel()) {
				m_stBaseData.tVipInvalidTime += nDay * 24 * 60 * 60;
			}
			else {
				m_stBaseData.tVipInvalidTime = time(nullptr) + nDay * 24 * 60 * 60;
			}
			m_stBaseData.nVipLevel = nVipLevel;
		}
		else {
			nRet = 8;
		}
	}
	else {
		m_stBaseData.nVipLevel = 0;
		m_stBaseData.tVipInvalidTime = 0;
	}

	m_bVipDataDirty = true;
	if (nRet == 0) {
		sendVipInfo();
	}
	LOGFMTD("uid = %u change vip ret = %u, final Level = %u , invalidTime = %u", getPlayer()->getUserUID(), nRet, m_stBaseData.nVipLevel, m_stBaseData.tVipInvalidTime);
	return nRet;
}

bool CPlayerBaseData::isOutVipCreateRoomLimit(uint32_t nRoomCnt) {
	sortVipInfo();

	auto nVipLevel = getVipLevel();
	if (nVipLevel) {
		nVipLevel = 1;
	}

	switch (nVipLevel)
	{
	case 1 :
	{
		return nRoomCnt >= 3;
	}
	break;
	}
	return false;
}