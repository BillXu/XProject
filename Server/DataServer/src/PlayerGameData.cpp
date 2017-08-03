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
	case eAsync_Request_CreateRoomInfo:
	{
		// { targetUID : 23 , sessionID : 23 }  // result : { ret : 0 , uid  23 , diamond : 23 , alreadyRoomCnt : 23 }  // ret : 0 success ,1 session id and uid not match  .2, not find target player 
		auto nSessionID = jsReqContent["sessionID"].asUInt();
		if (nSessionID != getPlayer()->getSessionID())
		{
			jsResult["ret"] = 1;
			LOGFMTE("player uid = %u create room session id not match",getPlayer()->getUserUID());
			break;
		}
		jsResult["uid"] = getPlayer()->getUserUID();
		jsResult["diamond"] = getPlayer()->getBaseData()->getDiamoned();
		jsResult["alreadyRoomCnt"] = m_vCreatedRooms.size();
	}
	break;
	case eAsync_Inform_CreatedRoom:
	{
		auto nRoomID = jsReqContent["roomID"].asUInt();
		m_vCreatedRooms.push_back(nRoomID);
		LOGFMTD("player uid = %u create Room id = %u , add to room list",getPlayer()->getUserUID(),nRoomID );
	}
	break;
	case eAsync_Inform_RoomDeleted:
	{
		auto nRoomID = jsReqContent["roomID"].asUInt();
		auto iter = std::find(m_vCreatedRooms.begin(),m_vCreatedRooms.end(),nRoomID );
		if (iter != m_vCreatedRooms.end())
		{
			m_vCreatedRooms.erase(iter);
			LOGFMTD( "uid = %u 's room deleted room id = %u",getPlayer()->getUserUID(),nRoomID );
			break;
		}
		LOGFMTE( "uid = %u don't have room id = %u , how delete ?",getPlayer()->getUserUID(),nRoomID );
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

	if (getStayInRoomID() == 0)
	{
		return;
	}

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
	return ID_MSG_PORT_POKER;
}

bool CPlayerGameData::canRemovePlayer()
{
	return 0 == getStayInRoomID();
}

bool CPlayerGameData::onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort)
{
	if ( MSG_ROOM_REQ_ROOM_LIST == nmsgType )
	{
		Json::Value jsRoomIDs;
		for (auto& ref : m_vCreatedRooms)
		{
			jsRoomIDs[jsRoomIDs.size()] = ref;
		}
		Json::Value jsMsg;
		jsMsg["ret"] = 0;
		jsMsg["stayInRoomID"] = m_nStayRoomID;
		jsMsg["roomIDS"] = jsRoomIDs;
		sendMsg(jsMsg, nmsgType);
		return true;
	}
	return false;
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