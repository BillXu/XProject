#include "RoomManager.h"
#include "GoldenPrivateRoom.h"
#include "CoinRedBlack\RedBlackRoom.h"
#include "ISeverApp.h"
#include "stEnterRoomData.h"
#include "CoinRedBlack\RedBlackCoinRoom.h"
void RoomManager::onConnectedSvr(bool isReconnected)
{
	IGameRoomManager::onConnectedSvr(isReconnected);
	if (!isReconnected)
	{
		for ( int8_t nL = eRoomLevel_0; nL < eRoomLevel_1; ++nL)
		{
			createCoinRoom(nL);
		}
	}
}

bool RoomManager::onPublicMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID)
{
	switch (nMsgType)
	{
	case MSG_ENTER_COIN_GAME:
	{
		auto nUserID = prealMsg["uid"].asUInt();
		auto nLevel = prealMsg["level"].asUInt();

		if (nLevel >= eRoomLevel_Max)
		{
			Json::Value jsRet;
			jsRet["ret"] = 4;
			sendMsg(jsRet, MSG_ENTER_COIN_GAME, nSenderID, nSenderID, ID_MSG_PORT_CLIENT);
			break;
		}

		Json::Value jsReq;
		jsReq["targetUID"] = nUserID;
		jsReq["level"] = nLevel;
		jsReq["sessionID"] = nSenderID;
		jsReq["port"] = getSvrApp()->getLocalSvrMsgPortType();
		jsReq["portIdx"] = getSvrApp()->getCurSvrIdx();
		auto pAsync = getSvrApp()->getAsynReqQueue();
		pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nUserID, eAsync_Request_EnterCoinGameInfo, jsReq, [pAsync, nLevel, nSenderID, this, nUserID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
		{
			if (isTimeOut)
			{
				LOGFMTE(" request time out uid = %u , can not enter coin room ", nUserID);
				Json::Value jsRet;
				jsRet["ret"] = 5;
				sendMsg(jsRet, MSG_ENTER_COIN_GAME, nSenderID, nSenderID, ID_MSG_PORT_CLIENT);
				return;
			}

			stEnterRoomData tInfo;
			uint8_t nRet = retContent["ret"].asInt();
			IGameRoom* pRoomToEnter = nullptr;
			do
			{
				if (nRet)
				{
					break;
				}

				tInfo.nUserUID = retContent["uid"].asUInt();
				tInfo.nSessionID = nSenderID;
				tInfo.nDiamond = retContent["diamond"].asUInt();
				tInfo.nChip = retContent["coin"].asUInt();

				pRoomToEnter = getCoinRoomToEnterByLevel(nLevel);
				if ( nullptr == pRoomToEnter)
				{
					nRet = 9;
					LOGFMTE( "room is null ptr level = %u",nLevel );
					break;
				}

				if ( pRoomToEnter->checkPlayerCanEnter(&tInfo) )
				{
					nRet = 8;
					break;
				}

			} while (0);

			Json::Value jsRet;
			jsRet["gamePort"] = getSvrApp()->getLocalSvrMsgPortType();
			jsRet["level"] = nLevel;
			jsRet["ret"] = nRet;
			sendMsg(jsRet, MSG_ENTER_COIN_GAME, nSenderID, nSenderID, ID_MSG_PORT_CLIENT);
			if (8 == nRet)
			{
				Json::Value jsReqLeave;
				jsReqLeave["targetUID"] = nUserID;
				pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nUserID, eAsync_Clear_Queuing_CoinGameLevel, jsReqLeave);
				return;
			}
			// push to room queue
			if (nRet == 0)
			{
				pRoomToEnter->onPlayerEnter(&tInfo);
				pRoomToEnter->sendRoomInfo(tInfo.nSessionID);

				Json::Value jsE;
				// update stayin roomID
				jsE["targetUID"] = tInfo.nUserUID;
				jsE["roomID"] = pRoomToEnter->getRoomID();
				jsE["port"] = getSvrApp()->getLocalSvrMsgPortType();
				pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, tInfo.nUserUID, eAsync_Inform_EnterRoom, jsE);
			}
		}, nUserID);
	}
	break;
	default:
		return IGameRoomManager::onPublicMsg(prealMsg, nMsgType, eSenderPort, nSenderID, nTargetID);
	}
	return true;
}

IGameRoom* RoomManager::getCoinRoomToEnterByLevel(uint8_t nLevel)
{
	for (auto& ref : m_vRooms)
	{
		auto p = dynamic_cast<ICoinRoom*>(ref.second);
		if ( p == nullptr )
		{
			continue;
		}

		if ( p->getLevel() == nLevel &&  false == p->isRoomFull())
		{
			return p;
		}
	}
	return createCoinRoom(nLevel);
}

IGameRoom* RoomManager::createRoom(uint8_t nGameType)
{
	if ( eGame_Golden == nGameType )
	{
		return new GoldenPrivateRoom();
	}
	LOGFMTE("unknown game type = %u , can not create private room", nGameType );
	return nullptr;
}

uint8_t RoomManager::getDiamondNeed(uint8_t nGameType, uint8_t nLevel, ePayRoomCardType payType)
{
	if (isCreateRoomFree())
	{
		return 0;
	}
#ifdef _DEBUG
	return 0;
#endif // _DEBUG

	if (nLevel >= 3)
	{
		LOGFMTE("invalid room level for game = %u , level = %u", nGameType, nLevel);
		nLevel = 2;
	}

	// is aa true ;
	if (ePayType_AA == payType)
	{
		uint8_t vAA[] = { 1 , 2 , 3 };
		return vAA[nLevel] * 10 * 2;
	}

	// 6,1 . 12.2 , 18. 3
	uint8_t vFangZhu[] = { 6 , 12 , 18 };
	return vFangZhu[nLevel] * 10 * 0.5 * 2;
}

IGameRoom* RoomManager::createCoinRoom(uint8_t nLevel)
{
	Json::Value vjs;

	Json::Value jsOpts;
	jsOpts["maxBet"] = 1000;
	jsOpts["gameType"] = eGame_Golden;
	jsOpts["seatCnt"] = 10;
	jsOpts["enterLimitLow"] = 0;
	jsOpts["enterLimitTop"] = 0;
	jsOpts["deskFee"] = 10;
	jsOpts["level"] = vjs.size();
	vjs[vjs.size()] = jsOpts;
	jsOpts["level"] = vjs.size();
	vjs[vjs.size()] = jsOpts;
	jsOpts["level"] = vjs.size();
	vjs[vjs.size()] = jsOpts;
	auto p = new class RedBlackCoinRoom();
	p->init(this, generateSieralID(), generateRoomID(), jsOpts["seatCnt"].asInt(), vjs[nLevel]);
	m_vRooms[p->getRoomID()] = p;
	return p;
}