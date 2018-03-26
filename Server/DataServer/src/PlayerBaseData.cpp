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
#include "Utility.h"
#pragma warning( disable : 4996 )
CPlayerBaseData::CPlayerBaseData(CPlayer* player )
	:IPlayerComponent(player)
{
	m_eType = ePlayerComponent_BaseData ;
	//memset(&m_stBaseData,0,sizeof(m_stBaseData)) ;
	m_stBaseData.resetZero();
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
	//memset(&m_stBaseData,0,sizeof(m_stBaseData)) ;
	m_stBaseData.resetZero();
	m_stBaseData.nUserUID = getPlayer()->getUserUID() ;
	m_isReadingDB = false;
	m_bPlayerInfoDirty = false;
	m_bMoneyDataDirty = false;
	m_nTmpCoin = 0;
	m_nTmpDiamond = 0;
}

void CPlayerBaseData::reset()
{
	m_bMoneyDataDirty = false;
	m_isReadingDB = false;
	m_bPlayerInfoDirty = false;
	m_nTmpCoin = 0;
	m_nTmpDiamond = 0;
	//memset(&m_stBaseData,0,sizeof(m_stBaseData)) ;
	m_stBaseData.resetZero();
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
	sprintf_s(pBuffer, "SELECT nickName,sex,coin,diamond,emojiCnt,headIcon,joinedClub,createdClub,allGame,winGame FROM playerbasedata where userUID = %u ;",getPlayer()->getUserUID());
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
		m_isReadingDB = false;
		if (jsData.size() == 0)
		{
			LOGFMTE("why read player uid = %u base data is null ? ",getPlayer()->getUserUID() );
			return;
		}

		Json::Value jsRow = jsData[0u];
		sprintf_s(m_stBaseData.cHeadiconUrl,"%s", jsRow["headIcon"].asCString() );
		sprintf_s(m_stBaseData.cName, "%s", jsRow["nickName"].asCString());
		m_stBaseData.nCoin = jsRow["coin"].asUInt();
		m_stBaseData.nDiamoned = jsRow["diamond"].asUInt();
		m_stBaseData.nEmojiCnt = jsRow["emojiCnt"].asUInt();
		m_stBaseData.nSex = jsRow["sex"].asUInt();
		m_stBaseData.nAllGame = jsRow["allGame"].asUInt();
		m_stBaseData.nWinGame = jsRow["winGame"].asUInt();

		std::string sJoinedClub = jsRow["joinedClub"].asCString();
		if (sJoinedClub.size()) {
			VEC_STRING vsJoinedClub;
			StringSplit(sJoinedClub, ".", vsJoinedClub);
			for (auto sClub : vsJoinedClub) {
				m_stBaseData.vJoinedClub.push_back((uint32_t)std::stoi(sClub));
			}
		}

		std::string sCreatedClub = jsRow["createdClub"].asCString();
		if (sCreatedClub.size()) {
			VEC_STRING vsCreatedClub;
			StringSplit(sCreatedClub, ".", vsCreatedClub);
			for (auto sClub : vsCreatedClub) {
				m_stBaseData.vCreatedClub.push_back((uint32_t)std::stoi(sClub));
			}
		}

		modifyMoney(m_nTmpCoin);
		modifyMoney(m_nTmpDiamond,true);
		m_nTmpCoin = 0;
		m_nTmpDiamond = 0;
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
		auto gd = (CPlayerGameData*)getPlayer()->getComponent(ePlayerComponent_PlayerGameData);
		if (gd->canRemovePlayer() == false) {
			Json::Value jsmsg;
			jsmsg["ret"] = 1;
			sendMsg(jsmsg, nmsgType);
			return true;
		}

		sprintf_s(m_stBaseData.cHeadiconUrl, "%s", recvValue["headIcon"].asCString());
		sprintf_s(m_stBaseData.cName, "%s", recvValue["name"].asCString());
		m_stBaseData.nSex = recvValue["sex"].asUInt();
		m_bPlayerInfoDirty = true;
		Json::Value jsmsg;
		jsmsg["ret"] = 0;
		sendMsg(jsmsg, nmsgType);
		return true;
	}
	else if ( MSG_PLAYER_UPDATE_GPS == nmsgType )
	{
		m_stBaseData.dfJ = recvValue["J"].asDouble();
		m_stBaseData.dfW = recvValue["W"].asDouble();
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
	else if (MSG_CLUB_PLAYER_CLUB_INFO == nmsgType) {
		Json::Value jsmsg, jsJoined, jsCreated;
		for (auto& ref : m_stBaseData.vJoinedClub) {
			jsJoined[jsJoined.size()] = ref;
		}
		for (auto& ref : m_stBaseData.vCreatedClub) {
			jsCreated[jsCreated.size()] = ref;
		}
		jsmsg["joined"] = jsJoined;
		jsmsg["created"] = jsCreated;
		sendMsg(jsmsg, nmsgType);
		return true;
	}

	return false ;
}

bool CPlayerBaseData::onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID) {
	if (eAsync_player_apply_DragIn_Clubs == nRequestType) {
		std::vector<uint32_t> vClubIDs;
		getJoinedAndCreatedClubs(vClubIDs);
		Json::Value jsClubIDs, jsReq;
		for (auto& ref : vClubIDs) {
			jsClubIDs[jsClubIDs.size()] = ref;
		}
		auto pApp = getPlayer()->getPlayerMgr()->getSvrApp();
		auto nLeagueID = jsReqContent["leagueID"].asUInt();
		jsReq["leagueID"] = nLeagueID;
		jsReq["clubIDs"] = jsClubIDs;

		pApp->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, nLeagueID, eAsync_league_apply_DragIn_Clubs, jsReq, [pApp, nReqSerial, nSenderPort, nSenderID, this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
		{
			Json::Value jsRet;
			if (isTimeOut)
			{
				LOGFMTE(" request of club from league apply drag in time out uid = %u , can not drag in ", getPlayer()->getUserUID());
				jsRet["ret"] = 7;
				pApp->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsRet, getPlayer()->getUserUID());
				return;
			}

			uint8_t nReqRet = retContent["ret"].asUInt();
			uint8_t nRet = 0;
			do {
				if (0 != nReqRet)
				{
					nRet = 4;
					break;
				}
				jsRet["clubIDs"] = retContent["clubIDs"];
			} while (0);

			jsRet["ret"] = nRet;
			pApp->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsRet, getPlayer()->getUserUID());
		});

		return true;
	}

	if (eAsync_player_apply_DragIn == nRequestType) {
		auto nClubID = jsReqContent["clubID"].asUInt();
		if (m_stBaseData.isInClub(nClubID) == false) {
			Json::Value jsBack;
			jsBack["ret"] = 1;
			getPlayer()->getPlayerMgr()->getSvrApp()->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsBack, getPlayer()->getUserUID());
			LOGFMTE("player apply drag in is not join this club ? uid = %u, clubID = %u ", getPlayer()->getUserUID(), nClubID);
			return true;
		}
		auto nAmount = jsReqContent["amount"].asUInt();
		if (nAmount == 0) {
			Json::Value jsBack;
			jsBack["ret"] = 2;
			getPlayer()->getPlayerMgr()->getSvrApp()->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsBack, getPlayer()->getUserUID());
			LOGFMTE("player apply drag in amount is missing ? uid = %u, clubID = %u ", getPlayer()->getUserUID(), nClubID);
			return true;
		}
		uint32_t nFee = nAmount / 10;
		if (nFee % 10 > 0) {
			nFee += 1;
		}
		uint32_t nReal = nAmount + nFee;
		if (nReal > m_stBaseData.nCoin) {
			Json::Value jsBack;
			jsBack["ret"] = 3;
			getPlayer()->getPlayerMgr()->getSvrApp()->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsBack, getPlayer()->getUserUID());
			LOGFMTE("player apply drag in amount is too much ? uid = %u, clubID = %u, amountReal = %u ", getPlayer()->getUserUID(), nClubID, nReal);
			return true;
		}
		uint32_t nRoomID = jsReqContent["roomID"].asUInt();
		auto pApp = getPlayer()->getPlayerMgr()->getSvrApp();
		Json::Value jsReq;
		jsReq["clubID"] = nClubID;
		jsReq["uid"] = getPlayer()->getUserUID();
		jsReq["amount"] = nAmount;
		jsReq["roomID"] = nRoomID;
		jsReq["port"] = nSenderPort;
		jsReq["leagueID"] = jsReqContent["leagueID"];
		jsReq["roomName"] = jsReqContent["roomName"];
		jsReq["roomLevel"] = jsReqContent["roomLevel"];
		pApp->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, nClubID, eAsync_club_apply_DragIn, jsReq, [pApp, nReqSerial, nRoomID, nSenderPort, nSenderID, this, nReal](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
		{
			Json::Value jsRet;
			if (isTimeOut)
			{
				LOGFMTE(" request of club apply drag in time out uid = %u , can not drag in ", getPlayer()->getUserUID());
				jsRet["ret"] = 8;
				pApp->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsRet, getPlayer()->getUserUID());
				return;
			}

			uint8_t nReqRet = retContent["ret"].asUInt();
			uint8_t nRet = 0;
			do {
				if (0 != nReqRet)
				{
					nRet = 8;
					break;
				}

				auto gd = (CPlayerGameData*)getPlayer()->getComponent(ePlayerComponent_PlayerGameData);
				gd->addDraginedRoom(CPlayerGameData::stRoomEntry(nRoomID, (eMsgPort)nSenderPort));
			} while (0);

			jsRet["ret"] = nRet;
			pApp->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsRet, getPlayer()->getUserUID());
		});
		return true;
	}
	return false;
}

void CPlayerBaseData::sendBaseDataToClient()
{
	if ( m_isReadingDB )
	{
		LOGFMTE( "player is reading from db can not send to client  , uid = %u",getPlayer()->getUserUID() );
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

	auto pStay = ((CPlayerGameData*)getPlayer()->getComponent(ePlayerComponent_PlayerGameData))->getStayInRoom();
	if (pStay.isEmpty() == false)
	{
		jsBaseData["stayRoomID"] = pStay.nRoomID;
	}
	sendMsg(jsBaseData, MSG_PLAYER_BASE_DATA);
	LOGFMTD("send msg to client base data uid = %u", getPlayer()->getUserUID() );
}

void CPlayerBaseData::timerSave()
{
	if ( m_isReadingDB == true )
	{
		return;
	}

	saveMoney();
	saveClub();
	saveGameWin();
	// check player info ;
	if ( m_bPlayerInfoDirty )
	{
		m_bPlayerInfoDirty = false;

		Json::Value jssql;
		char pBuffer[512] = { 0 };
		sprintf_s(pBuffer, "update playerbasedata set nickName = '%s' ,sex = %u , headIcon = '%s' where userUID = %u ;", getPlayerName(),getSex(),getHeadIcon(), getPlayer()->getUserUID());
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

void CPlayerBaseData::saveClub() {
	if (m_bClubDataDirty == false) {
		return;
	}
	m_bClubDataDirty = false;

	Json::Value jssql;
	char pBuffer[512] = { 0 };
	auto sJoined = m_stBaseData.jcToString();
	auto sCreated = m_stBaseData.ccToString();
	sprintf_s(pBuffer, "update playerbasedata set joinedClub = '%s' ,createdClub = '%s' where userUID = %u;", sJoined.c_str(), sCreated.c_str(), getPlayer()->getUserUID());
	std::string str = pBuffer;
	jssql["sql"] = pBuffer;
	auto pReqQueue = getPlayer()->getPlayerMgr()->getSvrApp()->getAsynReqQueue();
	pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, getPlayer()->getUserUID(), eAsync_DB_Update, jssql);
	LOGFMTD("uid = %u save joinedClub = %s , createdClub = %s", getPlayer()->getUserUID(), sJoined.c_str(), sCreated.c_str());
}

void CPlayerBaseData::saveGameWin() {
	if (m_bGameWinDirty == false) {
		return;
	}
	m_bGameWinDirty = false;
	Json::Value jssql;
	char pBuffer[512] = { 0 };
	sprintf_s(pBuffer, "update playerbasedata set allGame = %u ,winGame = %u where userUID = %u;", m_stBaseData.nAllGame, m_stBaseData.nWinGame, getPlayer()->getUserUID());
	std::string str = pBuffer;
	jssql["sql"] = pBuffer;
	auto pReqQueue = getPlayer()->getPlayerMgr()->getSvrApp()->getAsynReqQueue();
	pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, getPlayer()->getUserUID(), eAsync_DB_Update, jssql);
	LOGFMTD("uid = %u save allGame = %u , winGame = %u", getPlayer()->getUserUID(), m_stBaseData.nAllGame, m_stBaseData.nWinGame);
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

void CPlayerBaseData::addJoinedClub(uint32_t nClubID) {
	if (std::find(m_stBaseData.vJoinedClub.begin(), m_stBaseData.vJoinedClub.end(), nClubID) == m_stBaseData.vJoinedClub.end()) {
		m_stBaseData.vJoinedClub.push_back(nClubID);
		m_bClubDataDirty = true;
	}
}

void CPlayerBaseData::addCreatedClub(uint32_t nClubID) {
	if (std::find(m_stBaseData.vCreatedClub.begin(), m_stBaseData.vCreatedClub.end(), nClubID) == m_stBaseData.vCreatedClub.end()) {
		m_stBaseData.vCreatedClub.push_back(nClubID);
		m_bClubDataDirty = true;
	}
}

void CPlayerBaseData::removeJoinedClub(uint32_t nClubID) {
	auto it = std::find(m_stBaseData.vJoinedClub.begin(), m_stBaseData.vJoinedClub.end(), nClubID);
	if (it != m_stBaseData.vJoinedClub.end()) {
		m_stBaseData.vJoinedClub.erase(it);
		m_bClubDataDirty = true;
	}
}

void CPlayerBaseData::removeCreatedClub(uint32_t nClubID) {
	auto it = std::find(m_stBaseData.vCreatedClub.begin(), m_stBaseData.vCreatedClub.end(), nClubID);
	if (it != m_stBaseData.vCreatedClub.end()) {
		m_stBaseData.vCreatedClub.erase(it);
		m_bClubDataDirty = true;
	}
}

void CPlayerBaseData::dismissClub(uint32_t nClubID) {
	removeJoinedClub(nClubID);
	removeCreatedClub(nClubID);
}

void CPlayerBaseData::addGameWin(bool isWin) {
	m_stBaseData.nAllGame++;
	if (isWin) {
		m_stBaseData.nWinGame++;
	}
	m_bGameWinDirty = true;
}

void CPlayerBaseData::getJoinedAndCreatedClubs(std::vector<uint32_t>& vClubIDs) {
	vClubIDs.clear();
	vClubIDs.assign(m_stBaseData.vJoinedClub.begin(), m_stBaseData.vJoinedClub.end());
	vClubIDs.insert(vClubIDs.end(), m_stBaseData.vCreatedClub.begin(), m_stBaseData.vCreatedClub.end());
}