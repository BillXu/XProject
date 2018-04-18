#include "IGameRoomManager.h"
#include "IGameRoom.h"
#include "log4z.h"
#include "ISeverApp.h"
#include "AsyncRequestQuene.h"
#include "stEnterRoomData.h"
#include <ctime>
#include <algorithm>
#include "IPrivateRoom.h"
#define MAX_CREATE_ROOM_CNT 10
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
	case eAsync_HttpCmd_SetCreateRoomFee:
	{
		if (jsReqContent["isFree"].isNull() || jsReqContent["isFree"].isUInt() == false)
		{
			jsResult["ret"] = 1;
			break;
		}

		m_isCreateRoomFree = jsReqContent["isFree"].asUInt() == 1;
		jsResult["ret"] = 0;
	}
	break;
	case eAsync_HttpCmd_SetCanCreateRoom:
	{
		if (jsReqContent["canCreateRoom"].isNull() || jsReqContent["canCreateRoom"].isUInt() == false)
		{
			jsResult["ret"] = 1;
			break;
		}

		m_isCanCreateRoom = jsReqContent["canCreateRoom"].asUInt() == 1;
		jsResult["ret"] = 0;
	}
	break;
	case eAsync_HttpCmd_GetSvrInfo:
	{
		jsResult["ret"] = 0;
		// can create room ;
		jsResult["canCreateRoom"] = isCanCreateRoom() ? 1 : 0;
		// is free ;
		jsResult["isForFree"] = isCreateRoomFree() ? 1 : 0;
		// room cnt ;
		jsResult["roomCnt"] = m_vRooms.size();

		uint32_t nPlayerCnt = 0;
		uint32_t nActiveRoomCnt = 0;
		// active Room ;
		// playerCnt ;
		for ( auto& ref : m_vRooms )
		{
			if ( ref.second->getPlayerCnt() > 2 )
			{
				++nActiveRoomCnt;
			}
			nPlayerCnt = ref.second->getPlayerCnt();
		}
		jsResult["playerCnt"] = nPlayerCnt;
		jsResult["liveRoomCnt"] = nActiveRoomCnt;
	}
	break;
	case eAsync_HttpCmd_DismissRoom:
	{
		if (jsReqContent["roomID"].isNull() || jsReqContent["roomID"].isInt() == false)
		{
			jsResult["ret"] = 1;
			break;
		}

		auto nRoomID = jsReqContent["roomID"].asUInt();
		if ( nRoomID == 1 && isCanCreateRoom() )
		{
			jsResult["ret"] = 2;
			break;
		}

		if (nRoomID != 1)
		{
			auto p = getRoomByID(nRoomID);
			if (p == nullptr)
			{
				jsResult["ret"] = 3;
				break;
			}

			// do dissmiss
			Json::Value js;
			js["uid"] = 0;
			p->onMsg(js, MSG_APPLY_DISMISS_VIP_ROOM,ID_MSG_PORT_CLIENT,0 );
			jsResult["ret"] = 0;
			break;
		}

		// dismiss all 
		for (auto& ref : m_vRooms)
		{
			if (ref.second)
			{
				Json::Value js;
				js["uid"] = 0;
				ref.second->onMsg(js, MSG_APPLY_DISMISS_VIP_ROOM, ID_MSG_PORT_CLIENT, 0);
			}
		}
		jsResult["ret"] = 0;
	}
	break;
	case eAsync_ClubCreateRoom:
	{
		jsResult["ret"] = 0;
		if (false == isCanCreateRoom())
		{
			LOGFMTE(" svr maintenance , can not create room ,please create later");
			jsResult["ret"] = 2;
			break;
		}

		uint32_t nClubID = jsReqContent["clubID"].asUInt();
		uint32_t nClubDiamond = jsReqContent["diamond"].asUInt();
		auto nRoomType = jsReqContent["gameType"].asUInt();
		auto nLevel = jsReqContent["level"].asUInt();

		uint8_t nPayType = jsReqContent["payType"].asUInt();
		if (nPayType > ePayType_Max)
		{
			Assert(0, "invalid pay type value ");
			nPayType = ePayType_RoomOwner;
		}
		auto nDiamondNeed = 0;
		if ( nPayType == ePayType_RoomOwner && false == isCreateRoomFree() )
		{
			nDiamondNeed = getDiamondNeed(nRoomType, nLevel, (ePayRoomCardType)nPayType);
		}

		if ( nClubDiamond < nDiamondNeed)
		{
			jsResult["ret"] = 1;
			break;
		}

		// do create room ;
		auto pRoom = createRoom(nRoomType);
		if ( !pRoom )
		{
			jsResult["ret"] = 3;
			LOGFMTE("game room type is null , club id = %u create room failed", nClubID );
			break;
		}

		auto nNewRoomID = generateRoomID();
		if ( nNewRoomID == 0 )
		{
			jsResult["ret"] = 4;
			LOGFMTE("game room type is null , club id = %u create room failed", nClubID);
			delete pRoom;
			pRoom = nullptr;
			break;
		}

		Json::Value jscreateOpts = jsReqContent;
		pRoom->init(this, generateSieralID(), nNewRoomID, jsReqContent["seatCnt"].asUInt(), jscreateOpts);
		m_vRooms[pRoom->getRoomID()] = pRoom;
		
		jsResult["roomID"] = nNewRoomID;
		jsResult["diamondFee"] = nDiamondNeed;
		jsResult["roomIdx"] = jsReqContent["roomIdx"];
	}
	break;
	case eAsync_ClubDismissRoom:
	{
		uint32_t nRoomID = jsReqContent["roomID"].asUInt();

		auto pRoom = dynamic_cast<IPrivateRoom*>(getRoomByID(nRoomID));
		if (pRoom && pRoom->isClubRoom())
		{
			pRoom->doRoomGameOver(true);
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
		auto pCheckEnterRoomInfo = [this]( uint32_t nRoomID, uint32_t nUserID, uint32_t nSenderID )
		{
			Json::Value jsReq;
			jsReq["targetUID"] = nUserID;
			jsReq["roomID"] = nRoomID;
			jsReq["sessionID"] = nSenderID;
			jsReq["port"] = getSvrApp()->getLocalSvrMsgPortType();
			auto pAsync = getSvrApp()->getAsynReqQueue();
			pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nUserID, eAsync_Request_EnterRoomInfo, jsReq, [pAsync, nRoomID, nSenderID, this, nUserID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
			{
				if (isTimeOut)
				{
					LOGFMTE(" request time out uid = %u , can not enter room ", nUserID);
					Json::Value jsRet;
					jsRet["ret"] = 5;
					sendMsg(jsRet, MSG_ENTER_ROOM, nSenderID, nSenderID, ID_MSG_PORT_CLIENT);
					return;
				}

				uint8_t nReqRet = retContent["ret"].asUInt();
				uint8_t nRet = 0;
				do
				{
					if (1 == nReqRet)
					{
						nRet = 2;
						break;
					}

					if (2 == nReqRet)
					{
						nRet = 4;
						break;
					}

					if (0 != nReqRet)
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
					if (isAlreadyInThisRoom == false && pRoom->isRoomFull())
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
					if (isAlreadyInThisRoom == false && nRet)
					{
						break;
					}

					if (pRoom->onPlayerEnter(&tInfo))
					{
						pRoom->sendRoomInfo(tInfo.nSessionID);
					}

				} while (0);

				Json::Value jsRet;
				if (nRet)
				{
					jsRet["ret"] = nRet;
					sendMsg(jsRet, MSG_ENTER_ROOM, nSenderID, nSenderID, ID_MSG_PORT_CLIENT);

					if (nReqRet == 0) // must reset stay in room id ;
					{
						Json::Value jsReqLeave;
						jsReqLeave["targetUID"] = nUserID;
						jsReqLeave["roomID"] = nRoomID;
						jsReqLeave["port"] = getSvrApp()->getLocalSvrMsgPortType();
						pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nUserID, eAsync_Inform_Player_LeavedRoom, jsReqLeave);
					}
					return;
				}
				jsRet["ret"] = 0;
				sendMsg(jsRet, MSG_ENTER_ROOM, nSenderID, nSenderID, ID_MSG_PORT_CLIENT);
			}, nUserID);
		};

		auto pRoom = dynamic_cast<IPrivateRoom*>(getRoomByID(nRoomID));
		if ( pRoom && pRoom->isClubRoom() )
		{
			// check club members 
			Json::Value jsReq;
			jsReq["clubID"] = pRoom->getClubID();
			jsReq["uid"] = nUserID;
			auto pAsync = getSvrApp()->getAsynReqQueue();
			pAsync->pushAsyncRequest(ID_MSG_PORT_CLUB, pRoom->getClubID(), eAsync_ClubCheckMember, jsReq, [ pCheckEnterRoomInfo,nRoomID, nSenderID, this, nUserID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
			{
				if (isTimeOut)
				{
					LOGFMTE(" request check club time out uid = %u , can not enter room ", nUserID);
					Json::Value jsRet;
					jsRet["ret"] = 5;
					sendMsg(jsRet, MSG_ENTER_ROOM, nSenderID, nSenderID, ID_MSG_PORT_CLIENT);
					return;
				}

				uint8_t nRet = retContent["ret"].asUInt();
				if (nRet)
				{
					Json::Value jsRet;
					jsRet["ret"] = nRet == 1 ? 9 : 10;
					sendMsg(jsRet, MSG_ENTER_ROOM, nSenderID, nSenderID, ID_MSG_PORT_CLIENT);
					LOGFMTE("club member check failed ret = %u uid = %u , can not enter room ", nRet, nUserID);
					return;
				}
				else
				{
					pCheckEnterRoomInfo(nRoomID, nUserID, nSenderID);
				}
			}, nUserID);
		}
		else
		{
			pCheckEnterRoomInfo(nRoomID,nUserID,nSenderID);
		}
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
	if ( m_vRoomIDs.empty() )
	{
		if ( m_vRooms.empty() )
		{
			prepareRoomIDs();
		}
		else
		{
			LOGFMTE( "no enough roomids " );
			return 0;
		}
	}

#ifdef _DEBUG
	LOGFMTD( "reserver roomIDs = %u, roomsCnt = %u, all = %u",m_vRoomIDs.size(),m_vRooms.size(), m_vRoomIDs.size() + m_vRooms.size());
#endif // _DEBUG

	auto nRoomID = m_vRoomIDs.front();
	m_vRoomIDs.pop_front();
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
		// recycle room ids 
		m_vRoomIDs.push_back(nDelete);
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
	uint32_t nMax = getSvrApp()->getLocalSvrMsgPortType() + 1 ;
	nMax = nMax << 27;

	ss.str("");     
	ss << "SELECT max(sieralNum) as sieralNum FROM roominfo where sieralNum <  " << nMax << ";";
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
		m_nMaxSieralID |= ((uint32_t)getSvrApp()->getLocalSvrMsgPortType()) << 27;
		LOGFMTD("sieralNum id  = %u", m_nMaxSieralID);
	});
}

void IGameRoomManager::onPlayerCreateRoom( Json::Value& prealMsg, uint32_t nSenderID )
{
	if (false == isCanCreateRoom())
	{
		LOGFMTE(" svr maintenance , can not create room ,please create later");
		Json::Value jsRet;
		jsRet["ret"] = 5;
		sendMsg(jsRet, MSG_CREATE_ROOM, nSenderID, nSenderID, ID_MSG_PORT_CLIENT);
		return;
	}
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
			jsRet["ret"] = 7;
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
			uint8_t nPayType = ePayType_RoomOwner;
			if (jsUserData["isAA"].isNull() == false)
			{
				if (jsUserData["isAA"].asUInt() == 1)
				{
					nPayType = 1;
				}
			}
			else
			{
				if (jsUserData["payType"].isNull())
				{
					Assert(0, "no payType key");
				}
				else
				{
					nPayType = jsUserData["payType"].asUInt();
					if ( nPayType > ePayType_Max )
					{
						Assert(0, "invalid pay type value ");
						nPayType = ePayType_RoomOwner;
					}
				}

			}
			auto isRoomOwnerPay = (nPayType == ePayType_RoomOwner);
#ifndef _DEBUG
			if (nAlreadyRoomCnt >= MAX_CREATE_ROOM_CNT)
			{
				nRet = 2;
				break;
			}
#endif // _DEBUG

			auto nDiamondNeed = getDiamondNeed(nRoomType,nLevel, (ePayRoomCardType)nPayType );
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

			auto nNewRoomID = generateRoomID();
			if (nNewRoomID == 0 )
			{
				nRet = 6;
				LOGFMTE("game room type is null , uid = %u create room failed", nUserID);
				break;
			}

			pRoom->init(this, generateSieralID(), nNewRoomID, jsUserData["seatCnt"].asUInt(), jsUserData);
			m_vRooms[pRoom->getRoomID()] = pRoom;
			nRoomID = pRoom->getRoomID();
			// inform do created room ;
			Json::Value jsInformCreatRoom;
			jsInformCreatRoom["targetUID"] = nUserID;
			jsInformCreatRoom["roomID"] = nRoomID;
			jsInformCreatRoom["port"] = pAsync->getSvrApp()->getLocalSvrMsgPortType();
			pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nUserID, eAsync_Inform_CreatedRoom, jsInformCreatRoom );
			 
			// consume diamond 
			if ( isRoomOwnerPay && nDiamondNeed > 0 )
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

bool IGameRoomManager::isCreateRoomFree()
{
	return m_isCreateRoomFree;
}

bool IGameRoomManager::isCanCreateRoom()
{
	return m_isCanCreateRoom;
}

void IGameRoomManager::prepareRoomIDs()
{
	// begin(2) , portTypeCrypt (2),commonNum(2)
	uint32_t nPortType = getSvrApp()->getLocalSvrMsgPortType();
	auto nCurSvrIdx = getSvrApp()->getCurSvrIdx();
	auto nCurSvrMaxCnt = getSvrApp()->getCurSvrMaxCnt();
	if (0 == nCurSvrMaxCnt)
	{
		LOGFMTE( "can not invoker here , cur svr max cn must not be zero ?" );
		nCurSvrMaxCnt = 1;
		nCurSvrIdx = 0;
	}

	for (uint32_t nBegin = 10; nBegin <= 99; ++nBegin)
	{
		for (uint32_t nComNum = 0; nComNum <= 99; ++nComNum)
		{
			int32_t nPortTypeCrypt = 0;
			if (nComNum >= 50)
			{
				nPortTypeCrypt = nComNum + nPortType;
			}
			else
			{
				nPortTypeCrypt = (nPortType + 100) - nComNum;
			}
			nPortTypeCrypt %= 100;

			uint32_t nRoomID = nBegin * 10000 + nPortTypeCrypt * 100 + nComNum;
			if (nRoomID % nCurSvrMaxCnt != nCurSvrIdx)
			{
				continue;
			}
			m_vRoomIDs.push_back(nRoomID);
		}
	}

	std::random_shuffle(m_vRoomIDs.begin(), m_vRoomIDs.end());
}

//uint32_t parePortTypte(uint32_t nRoomID)
//{
//	// begin(2) , portTypeCrypt (2),commonNum(2)
//	int32_t nComNum = nRoomID % 100;
//	int32_t portTypeCrypt = ((int32_t)(nRoomID / 100)) % 100;
//	if (nComNum >= 50)
//	{
//		portTypeCrypt = portTypeCrypt + 100 - nComNum;
//	}
//	else
//	{
//		portTypeCrypt = portTypeCrypt + 100 + nComNum;
//	}
//
//	return (portTypeCrypt %= 100);
//}