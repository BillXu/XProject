#include "RoomManager.h"
#include "ThirteenPrivateRoom.h"
#include "ISeverApp.h"
#include "AsyncRequestQuene.h"
IGameRoom* RoomManager::createRoom(uint8_t nGameType)
{
	if (eGame_Thirteen == nGameType )
	{
		return new ThirteenPrivateRoom();
	}
	LOGFMTE("unknown game type = %u , can not create private room", nGameType );
	return nullptr;
}

uint32_t RoomManager::getDiamondNeed(uint8_t nGameType, uint8_t nLevel, ePayRoomCardType payType)
{
	if (isCreateRoomFree())
	{
		return 0;
	}
#ifdef _DEBUG
	return 0;
#endif // _DEBUG

	uint8_t nAmountLevel = nLevel >> 4;
	uint8_t nTypeLevel = (nLevel << 4) >> 4;
	if (nTypeLevel > 4) {
		LOGFMTE("invalid room level for game = %u , level = %u", nGameType, nLevel);
		nTypeLevel = 4;
	}
	if (nAmountLevel > 5) {
		LOGFMTE("invalid room level for game = %u , level = %u", nGameType, nLevel);
		nTypeLevel = 5;
	}
	uint16_t vDiamond[] = { 30, 40, 60, 70, 110 };
	uint16_t vAmount[] = { 4, 20, 30, 50, 100, 200 };
	uint32_t nDiamondNeed = vDiamond[nTypeLevel];
	nDiamondNeed = nDiamondNeed * vAmount[nAmountLevel] / 4;
	return nDiamondNeed;
	//if (nLevel >= 3)
	//{
	//	LOGFMTE("invalid room level for game = %u , level = %u", nGameType, nLevel);
	//	nLevel = 2;
	//}

	//// is aa true ;
	//if (ePayType_AA == payType)
	//{
	//	uint8_t vAA[] = { 1 , 2 , 3 };
	//	return vAA[nLevel] * 10;
	//}

	//// 6,1 . 12.2 , 18. 3
	//uint8_t vFangZhu[] = { 6 , 12 , 18 };
	//return vFangZhu[nLevel] * 10;
}

IGameRoom* RoomManager::doPlayerCreateRoom(Json::Value& prealMsg, uint32_t nDiamondNeed, bool isRoomOwnerPay) {
	auto nRoomType = prealMsg["gameType"].asUInt();
	auto nUserID = prealMsg["uid"].asUInt();
	auto pRoom = createRoom(nRoomType);
	if (!pRoom)
	{
		LOGFMTE("game room type is null , uid = %u create room failed", nUserID);
		return pRoom;
	}

	auto nNewRoomID = generateRoomID();
	if (nNewRoomID == 0)
	{
		LOGFMTE("game room type is null , uid = %u create room failed", nUserID);
		delete pRoom;
		pRoom = nullptr;
		return pRoom;
	}

	pRoom->init(this, generateSieralID(), nNewRoomID, prealMsg["seatCnt"].asUInt(), prealMsg);
	m_vRooms[pRoom->getRoomID()] = pRoom;
	auto nRoomID = pRoom->getRoomID();
	auto pAsync = getSvrApp()->getAsynReqQueue();
	// inform do created room ;
	Json::Value jsInformCreatRoom;
	jsInformCreatRoom["targetUID"] = nUserID;
	jsInformCreatRoom["roomID"] = nRoomID;
	jsInformCreatRoom["port"] = pAsync->getSvrApp()->getLocalSvrMsgPortType();
	pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nUserID, eAsync_Inform_CreatedRoom, jsInformCreatRoom);
	auto nClubID = prealMsg["clubID"].asUInt();
	if (nClubID) {
		jsInformCreatRoom["clubID"] = nClubID;
		auto nLeagueID = prealMsg["leagueID"].asUInt();
		if (nLeagueID) {
			jsInformCreatRoom["leagueID"] = nLeagueID;
		}
		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nClubID, eAsync_Club_CreateRoom, jsInformCreatRoom);
	}

	// consume diamond 
	if (isRoomOwnerPay && nDiamondNeed > 0)
	{
		//eAsync_Consume_Diamond, // { playerUID : 23 , diamond : 23 , roomID :23, reason : 0 }  // reason : 0 play in room , 1 create room  ;
		Json::Value jsConsumDiamond;
		jsConsumDiamond["playerUID"] = nUserID;
		jsConsumDiamond["diamond"] = nDiamondNeed;
		jsConsumDiamond["roomID"] = nRoomID;
		jsConsumDiamond["reason"] = 1;
		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nUserID, eAsync_Consume_Diamond, jsConsumDiamond);
		LOGFMTD("user uid = %u agent create room do comuse diamond = %u room id = %u", nUserID, nDiamondNeed, nRoomID);
	}

	return pRoom;
}

void RoomManager::onPlayerCreateRoom(Json::Value& prealMsg, uint32_t nSenderID) {
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
	pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nUserID, eAsync_Request_CreateRoomInfo, jsReq, [pAsync, nSenderID, this, nUserID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
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
		uint32_t nClubID = 0;
		uint32_t nDiamondNeed = 0;
		bool isRoomOwnerPay = false;
		// { ret : 0 , uid  23 , diamond : 23 , alreadyRoomCnt : 23 }
		do
		{
			if (0 != nReqRet)
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
					if (nPayType > ePayType_Max)
					{
						Assert(0, "invalid pay type value ");
						nPayType = ePayType_RoomOwner;
					}
				}

			}
			isRoomOwnerPay = (nPayType == ePayType_RoomOwner);
#ifndef _DEBUG
			if (nAlreadyRoomCnt >= MAX_CREATE_ROOM_CNT)
			{
				nRet = 2;
				break;
			}
#endif // _DEBUG

			nDiamondNeed = getDiamondNeed(nRoomType, nLevel, (ePayRoomCardType)nPayType);
			if (nDiamond < nDiamondNeed)
			{
				nRet = 1;
				break;
			}

			nClubID = jsUserData["clubID"].asUInt();
			if (nClubID) {
				break;
			}
			//auto nLeagueID = jsUserData["leagueID"].asUInt();

			// do create room ;
			auto pRoom = doPlayerCreateRoom(jsUserData, nDiamondNeed, isRoomOwnerPay);
			if (!pRoom)
			{
				nRet = 4;
				LOGFMTE("game room type is null , uid = %u create room failed", nUserID);
				break;
			}
			nRoomID = pRoom->getRoomID();

		} while (0);

		auto nLeagueID = jsUserData["leagueID"].asUInt();
		if (nClubID && !nRet) {
			Json::Value jsReq;
			jsReq["uid"] = nUserID;
			jsReq["clubID"] = nClubID;
			jsReq["leagueID"] = nLeagueID;
			pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nClubID, eAsync_Club_CreateRoom_Check, jsReq, [pAsync, nSenderID, this, nUserID, nDiamondNeed, isRoomOwnerPay, nClubID, nLeagueID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
			{
				if (isTimeOut)
				{
					LOGFMTE(" request of club time out uid = %u , can not create room ", nUserID);
					Json::Value jsRet;
					jsRet["ret"] = 7;
					sendMsg(jsRet, MSG_CREATE_ROOM, nSenderID, nSenderID, ID_MSG_PORT_CLIENT);
					return;
				}
				uint8_t nReqRet = retContent["ret"].asUInt();
				uint8_t nRet = 0;
				uint32_t nRoomID = 0;
				do {
					if (0 != nReqRet)
					{
						nRet = 5;
						break;
					}

					auto pRoom = doPlayerCreateRoom(jsUserData, nDiamondNeed, isRoomOwnerPay);
					if (!pRoom)
					{
						nRet = 4;
						LOGFMTE("game room type is null , uid = %u create room failed", nUserID);
						break;
					}
					nRoomID = pRoom->getRoomID();
					/*Json::Value jsInformCreatRoom;
					jsInformCreatRoom["clubID"] = nClubID;
					jsInformCreatRoom["leagueID"] = nLeagueID;
					jsInformCreatRoom["roomID"] = nRoomID;
					jsInformCreatRoom["port"] = pAsync->getSvrApp()->getLocalSvrMsgPortType();
					pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nClubID, eAsync_Club_CreateRoom, jsInformCreatRoom);*/

				} while (0);

				Json::Value jsRet;
				jsRet["ret"] = nRet;
				jsRet["roomID"] = nRoomID;
				sendMsg(jsRet, MSG_CREATE_ROOM, nSenderID, nSenderID, ID_MSG_PORT_CLIENT);
				LOGFMTD("uid = %u create room ret = %u , room id = %u", nUserID, nRet, nRoomID);

			}, jsUserData, nUserID);
		}
		else {
			Json::Value jsRet;
			jsRet["ret"] = nRet;
			jsRet["roomID"] = nRoomID;
			sendMsg(jsRet, MSG_CREATE_ROOM, nSenderID, nSenderID, ID_MSG_PORT_CLIENT);
			LOGFMTD("uid = %u create room ret = %u , room id = %u", nUserID, nRet, nRoomID);
		}
	}, prealMsg, nUserID);
}

bool RoomManager::onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult) {
	if (IGameRoomManager::onAsyncRequest(nRequestType, jsReqContent, jsResult)) {
		return true;
	}
	if (eAsync_club_agree_DragIn == nRequestType) {
		auto nRoomID = jsReqContent["roomID"].asUInt();
		auto nUID = jsReqContent["uid"].asUInt();
		auto nAmount = jsReqContent["amount"].asUInt();
		auto pRoom = (ThirteenPrivateRoom*)getRoomByID(nRoomID);
		if (pRoom && pRoom->onPlayerDragIn(nUID, nAmount)) {
			jsResult["ret"] = 0;
		}
		else {
			jsResult["ret"] = 1;
		}
		return true;
	}
	return false;
}