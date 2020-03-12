#include "PlayerGameData.h"
#include "Player.h"
#include "log4z.h"
#include "PlayerBaseData.h"
#include <json/json.h>
#include "AutoBuffer.h"
#include "AsyncRequestQuene.h"
#include "PlayerManager.h"
#include "ISeverApp.h"
#include <algorithm>
#include "DataServerApp.h"
#include "Club.h"
#define MAX_WHITE_LIST_CNT 50
CPlayerGameData::~CPlayerGameData()
{

}

void CPlayerGameData::reset()
{
	m_vWhiteList.clear();
	m_isWhiteListDirty = false;
}

bool CPlayerGameData::onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)
{
	switch (nRequestType)
	{
	case eAsync_Inform_Player_LeavedRoom:
	{
		auto roomID = jsReqContent["roomID"].asUInt();
		auto nport = (eMsgPort)(jsReqContent["port"].asUInt());
		if (getStayInRoom().nRoomID == roomID && nport == getStayInRoom().nSvrPort )
		{
			setStayInRoom(stRoomEntry(0,ID_MSG_PORT_MAX));
			break;
		}
		LOGFMTE("uid = %u , not in room id = %u , how to leave ? real room id = %u , port = %u , port2 = %u",getPlayer()->getUserUID(),roomID, getStayInRoom().nRoomID , nport, getStayInRoom().nSvrPort );
	}
	break;
	case eAsync_Request_EnterRoomInfo:
	{
		auto nRoomID = jsReqContent["roomID"].asUInt();
		auto nSessionID = jsReqContent["sessionID"].asUInt();
		auto nPort = (eMsgPort)(jsReqContent["port"].asUInt());
		auto nClubID = jsReqContent["clubID"].asUInt();
		if ( getStayInRoom().isEmpty() == false && ( nRoomID != getStayInRoom().nRoomID || nPort != getStayInRoom().nSvrPort ))
		{
			jsResult["ret"] = 1;
			LOGFMTW("uid = %u already room id= %u ,can not enter room id = %u , port = %u , portS = %u",getPlayer()->getUserUID(), getStayInRoom().nRoomID,nRoomID,nPort,getStayInRoom().nSvrPort );
			break;
		}

		if (nSessionID != getPlayer()->getSessionID())
		{
			jsResult["ret"] = 2;
			LOGFMTW("uid = %u , session id error , can not enter room id = %u",getPlayer()->getUserUID(),nRoomID );
			break;
		}

		if (nClubID) {
			auto pClub = DataServerApp::getInstance()->getClubMgr()->getClub(nClubID);
			if (pClub == nullptr || pClub->getMember(getPlayer()->getUserUID()) == nullptr) {
				jsResult["ret"] = 3;
				LOGFMTW("uid = %u is not one of club id = %u, can not enter room id = %u", getPlayer()->getUserUID(), nClubID, nRoomID);
				break;
			}
			auto pMember = pClub->getMember(getPlayer()->getUserUID());
			if (pMember->nPlayTime == 1) {
				jsResult["ret"] = 4;
				break;
			}
		}

		jsResult["ret"] = 0;
		jsResult["uid"] = getPlayer()->getUserUID();
		jsResult["coin"] = getPlayer()->getBaseData()->getCoin();
		jsResult["diamond"] = getPlayer()->getBaseData()->getDiamoned();
		jsResult["stayRoomID"] = getStayInRoom().nRoomID;

		// do stay in this room ;
		setStayInRoom(stRoomEntry(nRoomID,nPort));
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
		uint32_t nRoomCnt = m_vCreatedRooms.size();
		if (getPlayer()->getBaseData()->isOutVipCreateRoomLimit(nRoomCnt)) {
			jsResult["ret"] = 8;
			LOGFMTE("player uid = %u create room out vip limit", getPlayer()->getUserUID());
			break;
		}
		jsResult["uid"] = getPlayer()->getUserUID();
		jsResult["diamond"] = getPlayer()->getBaseData()->getDiamoned();
		jsResult["alreadyRoomCnt"] = nRoomCnt;
		jsResult["vipLevel"] = getPlayer()->getBaseData()->getVipLevel();
	}
	break;
	case eAsync_Inform_CreatedRoom:
	{
		auto nRoomID = jsReqContent["roomID"].asUInt();
		auto nPort = (eMsgPort)(jsReqContent["port"].asUInt());
		m_vCreatedRooms.push_back(stRoomEntry(nRoomID,nPort));
		LOGFMTD("player uid = %u create Room id = %u , add to room list",getPlayer()->getUserUID(),nRoomID );
	}
	break;
	case eAsync_Inform_RoomDeleted:
	{
		auto nRoomID = jsReqContent["roomID"].asUInt();
		auto nPort = (eMsgPort)(jsReqContent["port"].asUInt());
		auto iter = std::find_if(m_vCreatedRooms.begin(), m_vCreatedRooms.end(), [nRoomID, nPort](stRoomEntry& ref) { return ref.nRoomID == nRoomID && nPort == ref.nSvrPort; });
		if (iter != m_vCreatedRooms.end())
		{
			m_vCreatedRooms.erase(iter);
			LOGFMTD( "uid = %u 's room deleted room id = %u",getPlayer()->getUserUID(),nRoomID );
			break;
		}
		LOGFMTE( "uid = %u don't have room id = %u , how delete ?",getPlayer()->getUserUID(),nRoomID );
	}
	break;
	case eAsync_Check_WhiteList:
	{
		auto nCheckUID = jsReqContent["checkUID"].asUInt();
		jsResult["ret"] = isUserIDInWhiteList(nCheckUID) ? 0 : 1;
	}
	break;
	default:
		return false;
	}
	return true;
}

void CPlayerGameData::setStayInRoom( stRoomEntry tRoom )
{
	m_tStayRoom = tRoom;
}

const CPlayerGameData::stRoomEntry& CPlayerGameData::getStayInRoom()
{
	return m_tStayRoom;
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

void CPlayerGameData::onPlayerOtherDeviceLogin(uint32_t nOldSessionID, uint32_t nNewSessionID)
{
	IPlayerComponent::onPlayerOtherDeviceLogin(nOldSessionID,nNewSessionID );

	auto& refStayInRoom = getStayInRoom();
	if (refStayInRoom.isEmpty() )
	{
		return;
	}

	auto pAsync = getPlayer()->getPlayerMgr()->getSvrApp()->getAsynReqQueue();
	Json::Value jsReq;
	jsReq["roomID"] = refStayInRoom.nRoomID;
	jsReq["uid"] = getPlayer()->getUserUID();
	jsReq["newSessionID"] = nNewSessionID;
	pAsync->pushAsyncRequest( refStayInRoom.nSvrPort, refStayInRoom.nRoomID, eAsync_Inform_Player_NewSessionID, jsReq, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
	{
		if (isTimeOut)
		{
			LOGFMTE("inform player new session id  ,target svr crashed ,time out  room id = %u , uid = %u", getStayInRoom().nRoomID, getPlayer()->getUserUID());
			setStayInRoom(stRoomEntry(0,ID_MSG_PORT_MAX));
			return;
		}

		if (retContent["ret"].asUInt() != 0)
		{
			LOGFMTE("inform player new session id  ,can not find room id = %u , uid = %u", getStayInRoom().nRoomID, getPlayer()->getUserUID());
			setStayInRoom(stRoomEntry(0, ID_MSG_PORT_MAX));
			return;
		}
		LOGFMTD( "inform new state to game svr! uid = %u ",getPlayer()->getUserUID() );
	});
}


bool CPlayerGameData::canRemovePlayer()
{
	return getStayInRoom().isEmpty() && m_vCreatedRooms.empty();
}

void CPlayerGameData::adminVisitInfo(Json::Value& jsInfo)
{
	Json::Value jsRooms;
	for (auto& ref : m_vCreatedRooms)
	{
		Json::Value jsRoomEntry;
		jsRoomEntry["id"] = ref.nRoomID;
		jsRoomEntry["port"] = ref.nSvrPort;
		jsRooms[jsRooms.size()] = jsRoomEntry;
	}
	jsInfo["rooms"] = jsRooms;
	if (getStayInRoom().isEmpty() == false)
	{
		Json::Value jsRoomEntry;
		jsRoomEntry["id"] = getStayInRoom().nRoomID;
		jsRoomEntry["port"] = getStayInRoom().nSvrPort;
		jsInfo["stayInRoom"] = jsRoomEntry;
	}
}

bool CPlayerGameData::onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort)
{
	if ( MSG_ROOM_REQ_ROOM_LIST == nmsgType )
	{
		Json::Value jsRooms;
		for (auto& ref : m_vCreatedRooms)
		{
			Json::Value jsRoomEntry;
			jsRoomEntry["id"] = ref.nRoomID;
			jsRoomEntry["port"] = ref.nSvrPort;
			jsRooms[jsRooms.size()] = jsRoomEntry;
		}
		Json::Value jsMsg;
		jsMsg["ret"] = 0;
		jsMsg["rooms"] = jsRooms;
		if ( getStayInRoom().isEmpty() == false )
		{
			Json::Value jsRoomEntry;
			jsRoomEntry["id"] = getStayInRoom().nRoomID;
			jsRoomEntry["port"] = getStayInRoom().nSvrPort;
			jsMsg["stayInRoom"] = jsRoomEntry;
		}
		sendMsg(jsMsg, nmsgType);
		return true;
	}

	if ( MSG_ADD_WHILE_LIST == nmsgType )
	{
		if ( MAX_WHITE_LIST_CNT < m_vWhiteList.size() )
		{
			recvValue["ret"] = 1;
			sendMsg(recvValue,nmsgType);
			return true;
		}

		auto nUID = recvValue["uid"].asUInt();
		if ( nUID == 0 )
		{
			LOGFMTE( "add white list erro uid = 0" );
			return true;
		}
		m_vWhiteList.insert(nUID);
		m_isWhiteListDirty = true;
		recvValue["ret"] = 0;
		sendMsg(recvValue, nmsgType);
		return true;
	}

	if ( MSG_REMOVE_WHITE_LIST == nmsgType )
	{
		if (recvValue["uid"].isNull() || recvValue["uid"].isUInt() == false )
		{
			recvValue["ret"] = 3;
			sendMsg(recvValue, nmsgType);
			LOGFMTE(" MSG_REMOVE_WHITE_LIST argument error");
			return true;
		}

		auto nUID = recvValue["uid"].asUInt();
		if ( nUID == 0 )
		{
			LOGFMTE("remove white list erro uid = 0");
			return true;
		}
		auto nIter = std::find(m_vWhiteList.begin(), m_vWhiteList.end(), nUID);
		if (nIter == m_vWhiteList.end())
		{
			recvValue["ret"] = 1;
			sendMsg(recvValue, nmsgType);
			return true;
		}
		m_vWhiteList.erase(nIter);
		m_isWhiteListDirty = true;

		recvValue["ret"] = 0;
		sendMsg(recvValue, nmsgType);
		return true;
	}

	if ( MSG_REQUEST_WHITE_LIST == nmsgType )
	{
		Json::Value jsArray;
		for (auto& ref : m_vWhiteList)
		{
			jsArray[jsArray.size()] = ref;
		}
		Json::Value msg;
		msg["list"] = jsArray;
		sendMsg(msg,nmsgType);
		return true;
	}
	
	if ( MSG_PLAYER_RTI == nmsgType ) {
		bool bState = recvValue["state"].asBool();
		if (m_bRealTimeInformation == bState) {
			return true;
		}

		m_bRealTimeInformation = bState;
		return true;
	}
	return false;
}

void CPlayerGameData::onPlayerLogined()
{
	// do read from db 
	Json::Value jssql;
	std::ostringstream ssSql;
	ssSql << "select jsWhiteList from playerwhitelist where userUID = " << getPlayer()->getUserUID();
	auto s = ssSql.str();
	jssql["sql"] = ssSql.str();
	auto pAsyncQueu = getPlayer()->getPlayerMgr()->getSvrApp()->getAsynReqQueue();
	pAsyncQueu->pushAsyncRequest(ID_MSG_PORT_DB, getPlayer()->getUserUID(), eAsync_DB_Select, jssql, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut) 
	{
		if (isTimeOut)
		{
			LOGFMTE( "uid = %u read white list time out", getPlayer()->getUserUID());
			return;
		}

		uint32_t nAft = retContent["afctRow"].asUInt();
		auto jsData = retContent["data"];
		if (nAft == 0 || jsData.isNull())
		{
			return;
		}
		auto jsRow = jsData[(uint32_t)0];
		Json::Value jsWhiteList;
		Json::Reader jsReader;
		if ( !jsReader.parse(jsRow["jsWhiteList"].asString(), jsWhiteList))
		{
			LOGFMTE( "uid = %u paser jswhitelist error",getPlayer()->getUserUID() );
			return;
		}

		m_vWhiteList.clear();
		for (uint16_t nIdx = 0; nIdx < jsWhiteList.size(); ++nIdx)
		{
			m_vWhiteList.insert(jsWhiteList[nIdx].asUInt());
		}

	},getPlayer()->getUserUID());
}

bool CPlayerGameData::onOtherSvrShutDown(eMsgPort nSvrPort, uint16_t nSvrIdx, uint16_t nSvrMaxCnt)
{
	if (nSvrMaxCnt == 0)
	{
		LOGFMTE( "why svrMax cnt is 0 ? why shutdown ?  port = %u",nSvrPort );
		return true;
	}
	// check stayinRoom 
	if (getStayInRoom().isEmpty() == false && nSvrPort == getStayInRoom().nSvrPort && (getStayInRoom().nRoomID % nSvrMaxCnt) == nSvrIdx)
	{
		m_tStayRoom.clear();
	}

	// check create room ;
	for ( auto& ref : m_vCreatedRooms )
	{
		if (ref.nSvrPort == nSvrPort && (ref.nRoomID % nSvrMaxCnt) == nSvrIdx)
		{
			ref.clear();
		}
	}

	// remove empty 
	do
	{
		auto iter = std::find_if(m_vCreatedRooms.begin(), m_vCreatedRooms.end(), [](stRoomEntry& ref) { return ref.isEmpty(); });
		if (iter == m_vCreatedRooms.end())
		{
			break;
		}
		m_vCreatedRooms.erase(iter);
	} while (1);
	
	return false;
}

void CPlayerGameData::timerSave()
{
	if ( false == m_isWhiteListDirty )
	{
		return;
	}

	// construct msg ;
	m_isWhiteListDirty = false;
	Json::Value jsArray;
	for (auto& ref : m_vWhiteList)
	{
		jsArray[jsArray.size()] = ref;
	}

	Json::StyledWriter ss;
	auto jsDetail = ss.write(jsArray);

	Json::Value jssql;
	std::ostringstream ssSql;
	ssSql << "insert into playerwhitelist ( userUID,jsWhiteList) values (" << getPlayer()->getUserUID() << " ,'" << jsDetail << " ' ) ON DUPLICATE KEY UPDATE jsWhiteList = ' " << jsDetail << " ' ;";
	auto s = ssSql.str();
	jssql["sql"] = ssSql.str();
	auto pAsyncQueu = getPlayer()->getPlayerMgr()->getSvrApp()->getAsynReqQueue();
	pAsyncQueu->pushAsyncRequest(ID_MSG_PORT_DB, getPlayer()->getUserUID(), eAsync_DB_Add, jssql);
}

bool CPlayerGameData::isUserIDInWhiteList( uint32_t nUserUID )
{
	if (nUserUID == getPlayer()->getUserUID())
	{
		return true;
	}

	auto iter = std::find( m_vWhiteList.begin(),m_vWhiteList.end(),nUserUID );
	return iter != m_vWhiteList.end();
}

void CPlayerGameData::informNetState(uint8_t nStateFlag)
{
	if ( getStayInRoom().isEmpty() )
	{
		return;
	}

	auto pAsync = getPlayer()->getPlayerMgr()->getSvrApp()->getAsynReqQueue();
	Json::Value jsReq;
	jsReq["roomID"] = getStayInRoom().nRoomID;
	jsReq["uid"] = getPlayer()->getUserUID();
	jsReq["state"] = nStateFlag;
	pAsync->pushAsyncRequest(getStayInRoom().nSvrPort, getStayInRoom().nRoomID, eAsync_Inform_Player_NetState, jsReq, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
	{
		if (isTimeOut)
		{
			LOGFMTE("inform player net stae ,target svr crashed ,time out  room id = %u , uid = %u", getStayInRoom().nRoomID, getPlayer()->getUserUID());
			setStayInRoom(stRoomEntry(0,ID_MSG_PORT_MAX));
			return;
		}

		if (retContent["ret"].asUInt() != 0)
		{
			LOGFMTE("inform player net stae ,can not find room id = %u , uid = %u", getStayInRoom().nRoomID, getPlayer()->getUserUID());
			setStayInRoom(stRoomEntry(0, ID_MSG_PORT_MAX));
			return;
		}
		LOGFMTD("inform new state to game svr! uid = %u new", getPlayer()->getUserUID());
	});
}

void CPlayerGameData::sendIRT(Json::Value& jsMsg) {
	if (m_bRealTimeInformation) {
		sendMsg(jsMsg, MSG_PLAYER_REAL_TIME_INFORMATION);
	}
}