#include "IGameRoomManager.h"
#include "IGameRoom.h"
#include "log4z.h"
#include "ISeverApp.h"
#include "AsyncRequestQuene.h"
#include "stEnterRoomData.h"
#include <ctime>
#define MAX_CREATE_ROOM_CNT 5
IGameRoomManager::~IGameRoomManager()
{
	for (auto pair : m_vRooms)
	{
		delete pair.second;
	}
	m_vRooms.clear();
}

IGameRoom* IGameRoomManager::getRoomByID(uint32_t nRoomID)
{
	auto iter = m_vRooms.find(nRoomID);
	if (iter == m_vRooms.end())
	{
		return nullptr;
	}
	return iter->second;
}

bool IGameRoomManager::onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)
{
	switch ( nRequestType )
	{
	case eAsync_Inform_Player_NetState:
	{
		jsResult["ret"] = 0;
		auto nRoomID = jsReqContent["roomID"].asUInt();
		auto nUID = jsReqContent["uid"].asUInt();
		auto nState = jsReqContent["state"].asUInt();
		auto pRoom = getRoomByID(nRoomID);
		if (pRoom == nullptr)
		{
			jsResult["ret"] = 1;
			break;
		}

		if (!pRoom->onPlayerNetStateRefreshed(nUID, (eNetState)nState))  // can not find player ;
		{
			jsResult["ret"] = 1;
			break;
		}
	}
	break;
	case eAsync_Inform_Player_NewSessionID:
	{
		jsResult["ret"] = 0;
		auto nRoomID = jsReqContent["roomID"].asUInt();
		auto nUID = jsReqContent["uid"].asUInt();
		auto nSessionID = jsReqContent["newSessionID"].asUInt();
		auto pRoom = getRoomByID(nRoomID);
		if (pRoom == nullptr)
		{
			jsResult["ret"] = 1;
			break;
		}

		if (!pRoom->onPlayerSetNewSessionID(nUID, nSessionID ) )  // can not find player ;
		{
			jsResult["ret"] = 1;
			break;
		}
	}
	break;
	default:
		return false ;
	}
	return true;
}

bool IGameRoomManager::onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID)
{
	if (onPublicMsg(prealMsg, nMsgType, eSenderPort, nSenderID, nTargetID))
	{
		return true;
	}

	auto pRoom = getRoomByID(nTargetID);
	if (pRoom == nullptr)
	{
		prealMsg["ret"] = 200;
		sendMsg(prealMsg, nMsgType, nTargetID, nSenderID, ID_MSG_PORT_CLIENT);
		LOGFMTE("can not find room id = %u from id = %u , to process msg = %u",nTargetID,nSenderID,nMsgType );
		return true;
	}

	if (!pRoom->onMsg(prealMsg, nMsgType, eSenderPort, nSenderID))
	{
		prealMsg["ret"] = 201;
		sendMsg(prealMsg, nMsgType, nTargetID, nSenderID, ID_MSG_PORT_CLIENT);
		if ( prealMsg["roomState"].isNull() )
		{
			LOGFMTE("room state not be asign in IPrivateRoom  onMsg , why ?");
		}
		LOGFMTW("room id = %u can not process msg = %u ,from id = %u , state = %u",nTargetID,nMsgType,nSenderID, prealMsg["roomState"].asUInt() );
		return false;
	}
	return true;
}

bool IGameRoomManager::onPublicMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID)
{
	switch ( nMsgType )
	{
	case MSG_ENTER_ROOM:
	{
		auto nRoomID = prealMsg["roomID"].asUInt();
		auto nUserID = prealMsg["uid"].asUInt();
		// request enter room info 
		Json::Value jsReq;
		jsReq["targetUID"] = nUserID;
		jsReq["roomID"] = nRoomID;
		jsReq["sessionID"] = nSenderID;
		auto pAsync = getSvrApp()->getAsynReqQueue();
		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nUserID, eAsync_Request_EnterRoomInfo, jsReq, [pAsync,nRoomID,nSenderID, this, nUserID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
		{
			if (isTimeOut)
			{
				LOGFMTE(" request time out uid = %u , can not enter room ", nUserID );
				Json::Value jsRet;
				jsRet["ret"] = 5;
				sendMsg(jsRet, MSG_ENTER_ROOM, nSenderID,nSenderID,ID_MSG_PORT_CLIENT);
				return;
			}

			uint8_t nReqRet = retContent["ret"].asUInt();
			uint8_t nRet = 0;
			do
			{
				if ( 1 == nReqRet)
				{
					nRet = 2;
					break;
				}

				if ( 2 == nReqRet)
				{
					nRet = 4;
					break;
				}

				if ( 0 != nReqRet )
				{
					nRet = 6;
					break;
				}

				auto pRoom = getRoomByID(nRoomID);
				if (nullptr == pRoom)
				{
					nRet = 1;
					break;
				}

				auto nStatyRoomID = retContent["stayRoomID"].asUInt();
				bool isAlreadyInThisRoom = nStatyRoomID == nRoomID;
				if ( isAlreadyInThisRoom == false && pRoom->isRoomFull())
				{
					nRet = 3;
					break;
				}

				stEnterRoomData tInfo;
				tInfo.nUserUID = retContent["uid"].asUInt();
				tInfo.nSessionID = nSenderID;
				tInfo.nDiamond = retContent["diamond"].asUInt();
				tInfo.nChip = retContent["coin"].asUInt();

				nRet = pRoom->checkPlayerCanEnter(&tInfo);
				if ( isAlreadyInThisRoom == false &&  nRet )
				{
					break;
				}

				pRoom->onPlayerEnter(&tInfo);
			} while (0);

			Json::Value jsRet;
			if (nRet)
			{
				jsRet["ret"] = nRet;
				sendMsg(jsRet, MSG_ENTER_ROOM, nSenderID, nSenderID, ID_MSG_PORT_CLIENT);

				if ( nReqRet == 0 ) // must reset stay in room id ;
				{
					Json::Value jsReqLeave;
					jsReqLeave["targetUID"] = nUserID;
					jsReqLeave["roomID"] = nRoomID;
					pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nUserID, eAsync_Inform_Player_LeavedRoom, jsReqLeave);
				}
				return ;
			}
			jsRet["ret"] = 0;
			sendMsg(jsRet, MSG_ENTER_ROOM, nSenderID, nSenderID, ID_MSG_PORT_CLIENT);
		},nUserID);
	}
	break;
	case MSG_CREATE_ROOM:
	{
		// req create player info 
		onPlayerCreateRoom(prealMsg,nSenderID );
	}
	break;
	default:
		return false; 
	}
	return true;
}

uint32_t IGameRoomManager::generateRoomID()
{
	uint32_t nRoomID = 0;
	uint32_t nTryTimes = 0;
	UINT32 nBase = 100000;
	do
	{
		uint32_t nLastID = (uint32_t)time(nullptr) % ( nBase / 100 );
		uint32_t nFirstID = rand() % 9 + 1; // [ 1- 9 ]
		uint32_t nSecondID = rand() % 100;
		nRoomID = nFirstID * nBase + nSecondID * ( nBase / 100 ) + nLastID;

		++nTryTimes;
		if ( nTryTimes > 1 )
		{
			LOGFMTD("try times = %u to generate room id ", nTryTimes);
			if ( nTryTimes > 999 )
			{
				LOGFMTE("try to many times ");
				nBase *= 10;
				nTryTimes = 1;
			}
		}
		
		if ( getRoomByID(nRoomID) == nullptr)
		{
			return nRoomID;
		}

	} while ( 1 );

	return nRoomID;
}

uint32_t IGameRoomManager::generateSieralID()
{
	m_nMaxSieralID += getSvrApp()->getCurSvrMaxCnt();
	return m_nMaxSieralID;
}

uint32_t IGameRoomManager::generateReplayID()
{
	m_nMaxReplayUID += getSvrApp()->getCurSvrMaxCnt();
	return m_nMaxReplayUID;
}

void IGameRoomManager::update(float fDeta)
{
	for (auto& pair : m_vRooms)
	{
		pair.second->update(fDeta);
	}

	for ( auto nDelete : m_vWillDeleteRoom )
	{
		auto iter = m_vRooms.find(nDelete);
		if ( iter != m_vRooms.end() )
		{
			iter->second->doDeleteRoom();

			delete iter->second;
			iter->second = nullptr;
			m_vRooms.erase(iter);
		}
	}
	m_vWillDeleteRoom.clear();
}

void IGameRoomManager::deleteRoom( uint32_t nRoomID )
{
	m_vWillDeleteRoom.push_back(nRoomID);
}

void IGameRoomManager::onConnectedSvr(bool isReconnected)
{
	if ( isReconnected )
	{
		return;
	}

	auto asyq = getSvrApp()->getAsynReqQueue();

	// read max replay id ;
	std::ostringstream ss;
	ss << "SELECT max(replayID) as maxReplayID FROM gamereplay; ";
	Json::Value jsReq;
	jsReq["sql"] = ss.str();
	asyq->pushAsyncRequest(ID_MSG_PORT_RECORDER_DB,getSvrApp()->getCurSvrIdx(), eAsync_DB_Select, jsReq, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut ) {
		uint32_t nAft = retContent["afctRow"].asUInt();
		auto jsData = retContent["data"];
		if (nAft == 0 || jsData.isNull())
		{
			LOGFMTW("read maxReplayID id error, but no matter ");
			return;
		}

		auto jsRow = jsData[(uint32_t)0];
		m_nMaxReplayUID = jsRow["maxReplayID"].asUInt();
		m_nMaxReplayUID -= m_nMaxReplayUID % getSvrApp()->getCurSvrMaxCnt();
		m_nMaxReplayUID += getSvrApp()->getCurSvrIdx();
		LOGFMTD("maxReplayID id  = %u", m_nMaxReplayUID);
	});

	// read max room sieral num 
	ss.str("");     
	ss << "SELECT max(sieralNum) as sieralNum FROM roomrecorder ;";
	jsReq["sql"] = ss.str();
	asyq->pushAsyncRequest(ID_MSG_PORT_RECORDER_DB, getSvrApp()->getCurSvrIdx(), eAsync_DB_Select, jsReq, [this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut) {
		uint32_t nAft = retContent["afctRow"].asUInt();
		auto jsData = retContent["data"];
		if (nAft == 0 || jsData.isNull())
		{
			LOGFMTW("read sieralNum id error, but no matter ");
			return;
		}

		auto jsRow = jsData[(uint32_t)0];
		m_nMaxSieralID = jsRow["sieralNum"].asUInt();
		m_nMaxSieralID -= m_nMaxSieralID % getSvrApp()->getCurSvrMaxCnt();
		m_nMaxSieralID += getSvrApp()->getCurSvrIdx();
		LOGFMTD("sieralNum id  = %u", m_nMaxSieralID);
	});
}

void IGameRoomManager::onPlayerCreateRoom( Json::Value& prealMsg, uint32_t nSenderID )
{
	// request create RoomID room info 
	auto nUserID = prealMsg["uid"].asUInt();
	Json::Value jsReq;
	jsReq["sessionID"] = nSenderID;
	jsReq["targetUID"] = nUserID;
	auto pAsync = getSvrApp()->getAsynReqQueue();
	pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nUserID, eAsync_Request_CreateRoomInfo, jsReq, [pAsync,nSenderID, this, nUserID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
	{
		if (isTimeOut)
		{
			LOGFMTE(" request time out uid = %u , can not create room ", nUserID);
			Json::Value jsRet;
			jsRet["ret"] = 4;
			sendMsg(jsRet, MSG_CREATE_ROOM, nSenderID, nSenderID, ID_MSG_PORT_CLIENT);
			return;
		}

		uint8_t nReqRet = retContent["ret"].asUInt();
		uint8_t nRet = 0;
		uint32_t nRoomID = 0;
		// { ret : 0 , uid  23 , diamond : 23 , alreadyRoomCnt : 23 }
		do
		{
			if ( 0 != nReqRet )
			{
				nRet = 3;
				break;
			}

			jsUserData["uid"] = retContent["uid"];
			auto nDiamond = retContent["diamond"].asUInt();
			auto nAlreadyRoomCnt = retContent["alreadyRoomCnt"].asUInt();

			auto nRoomType = jsUserData["gameType"].asUInt();
			auto nLevel = jsUserData["level"].asUInt();
			auto isAA = jsUserData["isAA"].asUInt() == 1 ;
			auto isForFree = jsUserData["isFree"].asUInt() == 1;

			if (nAlreadyRoomCnt >= MAX_CREATE_ROOM_CNT)
			{
				nRet = 2;
				break;
			}
			auto nDiamondNeed = getDiamondNeed(nRoomType,nLevel,isAA);
			if ( nDiamond < nDiamondNeed )
			{
				nRet = 1;
				break;
			}

			// do create room ;
			auto pRoom = createRoom(nRoomType);
			if (!pRoom)
			{
				nRet = 4;
				LOGFMTE("game room type is null , uid = %u create room failed",nUserID );
				break;
			}
			pRoom->init(this, generateSieralID(), generateRoomID(), jsUserData["seatCnt"].asUInt(), jsUserData);
			m_vRooms[pRoom->getRoomID()] = pRoom;
			nRoomID = pRoom->getRoomID();
			// inform do created room ;
			Json::Value jsInformCreatRoom;
			jsInformCreatRoom["targetUID"] = nUserID;
			jsInformCreatRoom["roomID"] = nRoomID;
			pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nUserID, eAsync_Inform_CreatedRoom, jsInformCreatRoom );
			 
			// consume diamond 
			if ( isAA == false && isForFree == false )
			{
				//eAsync_Consume_Diamond, // { playerUID : 23 , diamond : 23 , roomID :23, reason : 0 }  // reason : 0 play in room , 1 create room  ;
				Json::Value jsConsumDiamond;
				jsConsumDiamond["playerUID"] = nUserID;
				jsConsumDiamond["diamond"] = nDiamondNeed;
				jsConsumDiamond["roomID"] = nRoomID;
				jsConsumDiamond["reason"] = 1;
				pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nUserID, eAsync_Consume_Diamond, jsConsumDiamond );
				LOGFMTD( "user uid = %u agent create room do comuse diamond = %u room id = %u",nUserID,nDiamondNeed,nRoomID  );
			}
 
		} while (0);

		Json::Value jsRet;
		jsRet["ret"] = nRet;
		jsRet["roomID"] = nRoomID;
		sendMsg(jsRet, MSG_CREATE_ROOM, nSenderID, nSenderID, ID_MSG_PORT_CLIENT);
		LOGFMTD("uid = %u create room ret = %u , room id = %u", nUserID,nRet,nRoomID );
	},prealMsg,nUserID );
}