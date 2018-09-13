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
	sprintf_s(pBuffer, "SELECT nickName,sex,coin,diamond,emojiCnt,headIcon,clubs,takeCharityTimes,lastTakeCardGiftTime FROM playerbasedata where userUID = %u ;",getPlayer()->getUserUID());
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
		m_stBaseData.nTakeCharityTimes = jsRow["takeCharityTimes"].asUInt();
		m_stBaseData.tLastTakeCardGiftTime = jsRow["lastTakeCardGiftTime"].asUInt();

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
	LOGFMTD("send msg to client base data uid = %u", getPlayer()->getUserUID() );
}

void CPlayerBaseData::timerSave()
{
	if ( m_isReadingDB == true )
	{
		return;
	}

	saveMoney();
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
		sprintf_s(pBuffer, "update playerbasedata set nickName = '%s' ,sex = %u , headIcon = '%s',clubs = '%s',takeCharityTimes = %u,lastTakeCardGiftTime = %u where userUID = %u ;", getPlayerName(),getSex(),getHeadIcon(), strClubs.c_str(), m_stBaseData.nTakeCharityTimes, m_stBaseData.tLastTakeCardGiftTime, getPlayer()->getUserUID());
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

bool CPlayerBaseData::canRemovePlayer() {
	if (m_vCreatedClubIDs.size()) {
		return false;
	}
	return true;
}