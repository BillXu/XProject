#include "PlayerGameData.h"
#include "Player.h"
#include "log4z.h"
#include "PlayerBaseData.h"
#include <json/json.h>
#include "AutoBuffer.h"
#include "AsyncRequestQuene.h"
#include "PlayerManager.h"
#include "ISeverApp.h"
CPlayerGameData::~CPlayerGameData()
{

}

bool CPlayerGameData::onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)
{
	switch (nRequestType)
	{
	case eAsync_Inform_Player_LeavedRoom:
	{
		auto roomID = jsReqContent["roomID"].asUInt();
		if (getStayInRoomID() == roomID)
		{
			setStayInRoomID(0);
			break;
		}
		LOGFMTE("uid = %u , not in room id = %u , how to leave ? real room id = %u",getPlayer()->getUserUID(),roomID, getStayInRoomID() );
	}
	break;
	case eAsync_Request_EnterRoomInfo:
	{
		auto nRoomID = jsReqContent["roomID"].asUInt();
		auto nSessionID = jsReqContent["sessionID"].asUInt();
		if (0 != getStayInRoomID() && nRoomID != getStayInRoomID())
		{
			jsResult["ret"] = 1;
			LOGFMTW("uid = %u already room id= %u ,can not enter room id = %u",getPlayer()->getUserUID(),getStayInRoomID(),nRoomID );
			break;
		}

		if (nSessionID != getPlayer()->getSessionID())
		{
			jsResult["ret"] = 2;
			LOGFMTW("uid = %u , session id error , can not enter room id = %u",getPlayer()->getUserUID(),nRoomID );
			break;
		}

		jsResult["ret"] = 0;
		jsResult["uid"] = getPlayer()->getUserUID();
		jsResult["coin"] = getPlayer()->getBaseData()->getCoin();
		jsResult["diamond"] = getPlayer()->getBaseData()->getDiamoned();

		// do stay in this room ;
		setStayInRoomID(nRoomID);
	}
	break;
	default:
		return false;
	}
	return true;
}

void CPlayerGameData::setStayInRoomID(uint32_t nRoomID)
{
	m_nStayRoomID = nRoomID;
}

uint32_t CPlayerGameData::getStayInRoomID()
{
	return m_nStayRoomID;
}

void CPlayerGameData::onPlayerDisconnect()
{
	IPlayerComponent::onPlayerDisconnect();
	informNetState(eNet_Offline);
}

void CPlayerGameData::onPlayerReconnected()
{
	IPlayerComponent::onPlayerReconnected();
	informNetState(eNet_Online);
}

void CPlayerGameData::onPlayerLoseConnect()
{
	IPlayerComponent::onPlayerLoseConnect();
	informNetState(eNet_WaitReconnect);
}

void CPlayerGameData::onPlayerOtherDeviceLogin(uint16_t nOldSessionID, uint16_t nNewSessionID)
{
	IPlayerComponent::onPlayerOtherDeviceLogin(nOldSessionID,nNewSessionID );

	auto pAsync = getPlayer()->getPlayerMgr()->getSvrApp()->getAsynReqQueue();
	Json::Value jsReq;
	jsReq["roomID"] = getStayInRoomID();
	jsReq["uid"] = getPlayer()->getUserUID();
	jsReq["newSessionID"] = nNewSessionID;
	pAsync->pushAsyncRequest(getGamePortByRoomID(getStayInRoomID()), getStayInRoomID(), eAsync_Inform_Player_NetState, jsReq, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
	{
		if (isTimeOut)
		{
			LOGFMTE("inform player new session id  ,target svr crashed ,time out  room id = %u , uid = %u", getStayInRoomID(), getPlayer()->getUserUID());
			setStayInRoomID(0);
			return;
		}

		if (retContent["ret"].asUInt() != 0)
		{
			setStayInRoomID(0);
			LOGFMTE("inform player new session id  ,can not find room id = %u , uid = %u", getStayInRoomID(), getPlayer()->getUserUID());
			return;
		}
	});
}

uint16_t CPlayerGameData::getGamePortByRoomID(uint32_t nRoomID)
{
	return ID_MSG_PORT_MJ;
}

bool CPlayerGameData::canRemovePlayer()
{
	return 0 == getStayInRoomID();
}

void CPlayerGameData::informNetState(uint8_t nStateFlag)
{
	if ( getStayInRoomID() == 0 )
	{
		return;
	}

	auto pAsync = getPlayer()->getPlayerMgr()->getSvrApp()->getAsynReqQueue();
	Json::Value jsReq;
	jsReq["roomID"] = getStayInRoomID();
	jsReq["uid"] = getPlayer()->getUserUID();
	jsReq["state"] = nStateFlag;
	pAsync->pushAsyncRequest(getGamePortByRoomID(getStayInRoomID()), getStayInRoomID(), eAsync_Inform_Player_NetState, jsReq, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
	{
		if (isTimeOut)
		{
			LOGFMTE("inform player net stae ,target svr crashed ,time out  room id = %u , uid = %u", getStayInRoomID(), getPlayer()->getUserUID());
			setStayInRoomID(0);
			return;
		}

		if (retContent["ret"].asUInt() != 0)
		{
			setStayInRoomID(0);
			LOGFMTE("inform player net stae ,can not find room id = %u , uid = %u", getStayInRoomID(), getPlayer()->getUserUID());
			return;
		}
	});
}