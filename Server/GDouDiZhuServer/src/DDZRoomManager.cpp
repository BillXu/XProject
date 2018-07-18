#include "DDZRoomManager.h"
#include "DDZPrivateRoom.h"
#include "ISeverApp.h"
#include "stEnterRoomData.h"
void DDZRoomManager::init(IServerApp* svrApp)
{
	IGameRoomManager::init(svrApp);

	Json::Value vjs;
	for ( uint16_t nLevel = eRoomLevel_0; nLevel < eRoomLevel_Max; ++nLevel)
	{
		m_vCoinRoomGroup[nLevel].init(this, vjs[nLevel]);
	}
}

void DDZRoomManager::update(float fDeta)
{
	IGameRoomManager::update(fDeta);
	for (uint16_t nLevel = eRoomLevel_0; nLevel < eRoomLevel_Max; ++nLevel)
	{
		m_vCoinRoomGroup[nLevel].update(fDeta);
	}
}

bool DDZRoomManager::onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)
{
	switch (nRequestType)
	{
	case eAsync_Inform_Player_NetState:
	{
		jsResult["ret"] = 0;
		auto nRoomID = jsReqContent["roomID"].asUInt();
		auto nUID = jsReqContent["uid"].asUInt();

		for (auto& ref : m_vCoinRoomGroup)
		{
			if (ref.removePlayerFromQueue(nUID))
			{
				return true;
			}
		}
		return IGameRoomManager::onAsyncRequest(nRequestType, jsReqContent, jsResult);
	}
	break;
	case eAsync_Inform_Player_NewSessionID:
	{
		jsResult["ret"] = 0;
		auto nRoomID = jsReqContent["roomID"].asUInt();
		auto nUID = jsReqContent["uid"].asUInt();
		auto nSessionID = jsReqContent["newSessionID"].asUInt();
		for (auto& ref : m_vCoinRoomGroup)
		{
			if (ref.updatePlayerSessionID(nUID,nSessionID))
			{
				return true;
			}
		}
		return IGameRoomManager::onAsyncRequest(nRequestType, jsReqContent, jsResult);
	}
	break;
	default:
		return IGameRoomManager::onAsyncRequest(nRequestType,jsReqContent,jsResult);
	}
	return true;
}

bool DDZRoomManager::onPublicMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID)
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

			auto nStayRoomID = retContent["stayRoomID"].asUInt();
			auto nAreadyLevel = retContent["level"].asInt();

			stEnterRoomData tInfo;
			uint8_t nRet = 0;
			do
			{
				if (nStayRoomID)
				{
					nRet = 2;
					break;
				}

				if (nAreadyLevel != -1 && nLevel != nAreadyLevel)
				{
					nRet = 3;
					break;
				}

				tInfo.nUserUID = retContent["uid"].asUInt();
				tInfo.nSessionID = nSenderID;
				tInfo.nDiamond = retContent["diamond"].asUInt();
				tInfo.nChip = retContent["coin"].asUInt();

				if (m_vCoinRoomGroup[nLevel].checkEnterThisLevel(tInfo))
				{
					nRet = 1;
					break;
				}

			} while (0);

			Json::Value jsRet;
			jsRet["ret"] = nRet;
			sendMsg(jsRet, MSG_ENTER_COIN_GAME, nSenderID, nSenderID, ID_MSG_PORT_CLIENT);
			if (0 == nRet)
			{
				Json::Value jsReqLeave;
				jsReqLeave["targetUID"] = nUserID;
				jsReqLeave["level"] = nLevel;
				jsReqLeave["port"] = getSvrApp()->getLocalSvrMsgPortType();
				pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nUserID, eAsync_Set_Queuing_CoinGameLevel, jsReqLeave);

				// push to room queue
				m_vCoinRoomGroup[nLevel].pushPlayerToEnterRoomQueue(tInfo);
			}

		}, nUserID);
	}
	break;
	default:
		return IGameRoomManager::onPublicMsg(prealMsg, nMsgType, eSenderPort, nSenderID, nTargetID);
	}
	return true;
}

IGameRoom* DDZRoomManager::getRoomByID(uint32_t nRoomID)
{
	auto p = IGameRoomManager::getRoomByID(nRoomID);
	if ( p )
	{
		return p;
	}

	for (auto& g : m_vCoinRoomGroup)
	{
		p = g.getRoomByID(nRoomID);
		if (p)
		{
			return p;
		}
	}

	return nullptr;
}

IGameRoom* DDZRoomManager::createRoom(uint8_t nGameType)
{
	if ( eGame_CYDouDiZhu == nGameType || eGame_JJDouDiZhu == nGameType )
	{
		return new DDZPrivateRoom();
	}
	LOGFMTE("unknown game type = %u , can not create private room", nGameType );
	return nullptr;
}

uint8_t DDZRoomManager::getDiamondNeed(uint8_t nGameType, uint8_t nLevel, ePayRoomCardType payType )
{
//#ifdef _DEBUG
//	return 0;
//#endif // _DEBUG

	if (isCreateRoomFree())
	{
		return 0;
	}

	if (nLevel >= 3)
	{
		LOGFMTE( "invalid room level for game = %u , level = %u",nGameType,nLevel );
		nLevel = 2;
	}

	// is aa true ;
	if (ePayType_AA == payType)
	{
		uint8_t vAA[] = { 1 , 2 , 3 };
		return vAA[nLevel] * 10;
	}

	// 6,1 . 12.2 , 18. 3
	uint8_t vFangZhu[] = { 3 , 6 , 9 };
	return vFangZhu[nLevel] * 10;
}