#include "PlayerBaseData.h"
#include <string>
#include "ServerMessageDefine.h"
#include "Player.h"
#include "log4z.h"
#include <time.h>
#include "GameServerApp.h"
#include "PlayerEvent.h"
#include "PlayerManager.h"
#include "EventCenter.h"
#include "AutoBuffer.h"
#include "AsyncRequestQuene.h"
#include <assert.h>
#pragma warning( disable : 4996 )
CPlayerBaseData::CPlayerBaseData(CPlayer* player )
	:IPlayerComponent(player)
{
	m_eType = ePlayerComponent_BaseData ;
	memset(&m_stBaseData,0,sizeof(m_stBaseData)) ;
	m_isReadingDB = false;
	m_bPlayerInfoDirty = false;
}

CPlayerBaseData::~CPlayerBaseData()
{

}

void CPlayerBaseData::init()
{
	memset(&m_stBaseData,0,sizeof(m_stBaseData)) ;
	m_stBaseData.nUserUID = getPlayer()->getUserUID() ;
	m_isReadingDB = false;
	m_bPlayerInfoDirty = false;
	m_bMoneyDataDirty = false;
}

void CPlayerBaseData::reset()
{
	m_bMoneyDataDirty = false;
	m_isReadingDB = false;
	m_bPlayerInfoDirty = false;
	memset(&m_stBaseData,0,sizeof(m_stBaseData)) ;
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
	sprintf_s(pBuffer, "SELECT nickName,sex,coin,diamond,headIcon FROM playerbasedata where userUID = %u ;",getPlayer()->getUserUID());
	std::string str = pBuffer;
	jssql["sql"] = pBuffer;
	auto pReqQueue = getPlayer()->getPlyerMgr()->getSvrApp()->getAsynReqQueue();
	pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, getPlayer()->getUserUID(), getPlayer()->getUserUID(), eAsync_DB_Select, jssql, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData)
	{
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
		m_stBaseData.nSex = jsRow["sex"].asUInt();

		sendBaseDataToClient();
	}
	);

	m_isReadingDB = true;
}

void CPlayerBaseData::onPlayerOtherDeviceLogin(uint16_t nOldSessionID, uint16_t nNewSessionID)
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
	jsBaseData["name"] = getPlayerName();
	jsBaseData["sex"] = getSex();
	jsBaseData["headIcon"] = getHeadIcon();
	jsBaseData["diamond"] = getDiamoned();
	jsBaseData["coin"] = getCoin();
	sendMsg(jsBaseData, MSG_PLAYER_BASE_DATA);
}

void CPlayerBaseData::timerSave()
{
	saveMoney();
	// check player info ;
	if ( m_bPlayerInfoDirty )
	{
		m_bPlayerInfoDirty = false;

		Json::Value jssql;
		char pBuffer[512] = { 0 };
		sprintf_s(pBuffer, "update playerbasedata set nickName = '%s' ,sex = %u , headIcon = '%s' where userUID = %u ;", getPlayerName(),getSex(),getHeadIcon(), getPlayer()->getUserUID());
		std::string str = pBuffer;
		jssql["sql"] = pBuffer;
		auto pReqQueue = getPlayer()->getPlyerMgr()->getSvrApp()->getAsynReqQueue();
		pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, getPlayer()->getUserUID(), getPlayer()->getUserUID(), eAsync_DB_Update, jssql);
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
	sprintf_s(pBuffer, "update playerbasedata set coin = %u ,diamond = %u where userUID = %u ;", getCoin(),getDiamoned(), getPlayer()->getUserUID());
	std::string str = pBuffer;
	jssql["sql"] = pBuffer;
	auto pReqQueue = getPlayer()->getPlyerMgr()->getSvrApp()->getAsynReqQueue();
	pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, getPlayer()->getUserUID(), getPlayer()->getUserUID(), eAsync_DB_Update, jssql);
	LOGFMTD( "uid = %u save money coin = %u , diamond = %u",getPlayer()->getUserUID(),getCoin(),getDiamoned() );
}

bool CPlayerBaseData::modifyMoney( int32_t nOffset, bool bDiamond )
{
	uint32_t& nRefMoney = bDiamond ? m_stBaseData.nDiamoned : m_stBaseData.nCoin;
	if ( nOffset < 0 )
	{
		if ((uint32_t)(nOffset * -1) > nRefMoney)
		{
			LOGFMTE("uid = %u money is too few , can not decrease coin = %u , diamond = %u , offset = %d", getPlayer()->getUserUID(), getCoin(), getDiamoned(), nOffset);
			return false;
		}
	}

	nRefMoney += nOffset;
	m_bMoneyDataDirty = true;
	return true;
}

