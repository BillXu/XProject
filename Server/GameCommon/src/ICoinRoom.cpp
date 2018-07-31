#include "ICoinRoom.h"
#include "log4z.h"
#include "IGameRoomManager.h"
#include "IGameRoomState.h"
#include "IGamePlayer.h"
#include "ISeverApp.h"
#include "AsyncRequestQuene.h"
ICoinRoom::~ICoinRoom()
{
	delete m_pRoom;
	m_pRoom = nullptr;
}

bool ICoinRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)
{
	m_pRoom = doCreatRealRoom();
	if (!m_pRoom)
	{
		LOGFMTE("create sys coin room error ");
		return false;
	}
	LOGFMTD("create 1 sys coin room");
	auto bRet = m_pRoom->init(pRoomMgr, nSeialNum, nRoomID, nSeatCnt, vJsOpts);
	if (!bRet)
	{
		LOGFMTE("init sys coin room error ");
		return false;
	}
	m_pRoom->setDelegate(this);
	m_vDelayStandUp.clear();
	m_vDelayLeave.clear();

	m_nEnterLimitLow = vJsOpts["enterLimitLow"].asInt();
	m_nEnterLimitTop = vJsOpts["enterLimitTop"].asInt();
	m_nDeskFee = vJsOpts["deskFee"].asInt();
	m_nLevel = vJsOpts["level"].asInt();
	return true;
}

uint8_t ICoinRoom::checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)
{
	if ( ( pEnterRoomPlayer->nChip > getEnterLimitTop() && getEnterLimitTop() != 0 ) || ( getEnterLimitLow() != 0 && pEnterRoomPlayer->nChip < getEnterLimitLow()) )
	{
		LOGFMTE( "room id = %u , out of limit chip uid = %u",getRoomID(),pEnterRoomPlayer->nUserUID );
		return 1;
	}
	return m_pRoom->checkPlayerCanEnter(pEnterRoomPlayer);
}

bool ICoinRoom::onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)
{
	if (m_pRoom == nullptr)
	{
		LOGFMTE("why room is null ? ");
		return false;
	}
	if (m_pRoom->onPlayerEnter(pEnterRoomPlayer))
	{

	}
	return true;
}

bool ICoinRoom::isRoomFull()
{
	return m_pRoom->isRoomFull();
}

bool ICoinRoom::doDeleteRoom() // wanning: invoke by roomMgr ;
{
	return m_pRoom->doDeleteRoom();
}

uint32_t ICoinRoom::getRoomID()
{
	return m_pRoom->getRoomID();
}

uint32_t ICoinRoom::getSeiralNum()
{
	return m_pRoom->getSeiralNum();
}

void ICoinRoom::update(float fDelta)
{
	if (m_pRoom)
	{
		m_pRoom->update(fDelta);
	}
}

bool ICoinRoom::onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)
{
	switch (nMsgType)
	{
	case MSG_REQUEST_ROOM_INFO:
	{
		LOGFMTD("reback room state and info msg to session id =%u", nSessionID);
		sendRoomInfo(nSessionID);
	}
	break;
	case MSG_PLAYER_STAND_UP:
	case MSG_PLAYER_LEAVE_ROOM:
	{
		auto pp = m_pRoom->getPlayerBySessionID(nSessionID);
		if ( pp && (pp->haveState(eRoomPeer_WaitNextGame) == false) ) // if game start , and you are sit down , you can not direct leave , if you not sit down , you can leave 
		{
			LOGFMTE("why you leave room ? already start can not leave , just apply dissmiss room id = %u , sessionID = %u", getRoomID(), nSessionID);
			decltype(m_vDelayLeave)& vDelay = MSG_PLAYER_STAND_UP == nMsgType ? m_vDelayStandUp : m_vDelayLeave;
			vDelay.insert(pp->getUserUID());

			prealMsg["ret"] = 2;
			sendMsgToPlayer(prealMsg, nMsgType, nSessionID);
			return true;
		}
		else
		{
			return m_pRoom->onMsg(prealMsg, nMsgType, eSenderPort, nSessionID);
		}
	}
	break;
	case MSG_PLAYER_CHAT_MSG:
	{
		auto pp = m_pRoom->getPlayerBySessionID(nSessionID);
		if (pp == nullptr)
		{
			prealMsg["ret"] = 1;
			sendMsgToPlayer(prealMsg, MSG_PLAYER_CHAT_MSG, nSessionID);
			break;
		}

		prealMsg["playerIdx"] = pp->getIdx();
		sendRoomMsg(prealMsg, MSG_ROOM_CHAT_MSG);
	}
	break;
	default:
		if (m_pRoom && m_pRoom->onMsg(prealMsg, nMsgType, eSenderPort, nSessionID))
		{
			
		}
		else
		{
			if (!m_pRoom)
			{
				LOGFMTE("why private room core Room is null");
				return false;
			}

			prealMsg["roomState"] = getCoreRoom()->getCurState()->getStateID();
			LOGFMTE("room id = %u do not process msg = %u , room state = %u idx = %u", getRoomID(), nMsgType, getCoreRoom()->getCurState()->getStateID(), getCoreRoom()->getCurState()->getCurIdx());
			return false;
		}
	}
	return true;
}

void ICoinRoom::sendRoomMsg(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nOmitSessionID  )
{
	if (m_pRoom)
	{
		m_pRoom->sendRoomMsg(prealMsg, nMsgType, nOmitSessionID);
	}
}

void ICoinRoom::sendMsgToPlayer(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nSessionID)
{
	if (m_pRoom)
	{
		m_pRoom->sendMsgToPlayer(prealMsg, nMsgType, nSessionID);
	}
}

void ICoinRoom::packRoomInfo(Json::Value& jsRoomInfo)
{
	if (m_pRoom)
	{
		m_pRoom->packRoomInfo(jsRoomInfo);
	}
}

void ICoinRoom::sendRoomPlayersInfo(uint32_t nSessionID)
{
	if (getCoreRoom())
	{
		getCoreRoom()->sendRoomPlayersInfo(nSessionID);
	}
}

void ICoinRoom::sendRoomInfo(uint32_t nSessionID)
{
	// send room info ;
	Json::Value jsMsg;
	packRoomInfo(jsMsg);
	LOGFMTI("send room info");
	sendMsgToPlayer(jsMsg, MSG_ROOM_INFO, nSessionID);

	// send players info ;
	sendRoomPlayersInfo(nSessionID);
}

bool ICoinRoom::onPlayerNetStateRefreshed(uint32_t nPlayerID, eNetState nState)
{
	auto pp = m_pRoom->getPlayerByUID(nPlayerID);
	if ( ( eNet_Offline == nState || eNet_WaitReconnect == nState ) && isDuringGame() && pp )
	{
		pp->setTuoGuanFlag(true);

		if ( eNet_Offline == nState )
		{
			LOGFMTE("player uid = %u do offline , but can not let sit down player leave room , room is started room id = %u", nPlayerID, getRoomID());
			m_vDelayLeave.insert(pp->getUserUID());
			return true;
		}

		Json::Value jsret;
		jsret["idx"] = pp->getIdx();
		jsret["isTuoGuan"] = 1;
		sendRoomMsg(jsret, MSG_DDZ_ROOM_UPDATE_TUO_GUAN);
	}
	return m_pRoom->onPlayerNetStateRefreshed(nPlayerID, nState);
}

bool ICoinRoom::onPlayerSetNewSessionID(uint32_t nPlayerID, uint32_t nSessinID)
{
	return m_pRoom->onPlayerSetNewSessionID(nPlayerID, nSessinID);
}

uint16_t ICoinRoom::getPlayerCnt()
{
	if (getCoreRoom() == nullptr)
	{
		return 0;
	}

	return getCoreRoom()->getPlayerCnt();
}

IGamePlayer* ICoinRoom::getPlayerByIdx(uint16_t nIdx)
{
	if (getCoreRoom() == nullptr)
	{
		return nullptr;
	}
	return getCoreRoom()->getPlayerByIdx(nIdx);
}

uint16_t ICoinRoom::getSeatCnt()
{
	if (getCoreRoom() == nullptr)
	{
		return 0;
	}

	return getCoreRoom()->getSeatCnt();
}

// delegate interface 
void ICoinRoom::onStartGame(IGameRoom* pRoom)
{
	// kou shui 
	for (auto nIdx = 0; nIdx < getSeatCnt(); ++nIdx)
	{
		auto p = getPlayerByIdx(nIdx);
		if (p == nullptr)
		{
			continue;
		}
		p->addSingleOffset(-1 * getDeskFee());
		m_nTotalFee += getDeskFee();
	}
}

bool ICoinRoom::canStartGame(IGameRoom* pRoom)
{
	return true;
}

void ICoinRoom::onGameDidEnd(IGameRoom* pRoom)
{
	 // delay stand up ;
	for (auto& ref : m_vDelayStandUp)
	{
		getCoreRoom()->doPlayerStandUp(ref);
	}
	m_vDelayStandUp.clear();

	// delay leave 
	for (auto& ref : m_vDelayLeave)
	{
		getCoreRoom()->doPlayerLeaveRoom(ref);
	}
	m_vDelayLeave.clear();
}

void ICoinRoom::onPlayerDoLeaved(IGameRoom* pRoom, uint32_t nUserUID)
{
	auto ps = getCoreRoom()->getStandPlayerByUID(nUserUID);
	if (nullptr == ps)
	{
		LOGFMTE( "why player uid = %u stand obj is null , room id = %u??",nUserUID,getRoomID() );
		return;
	}

	auto pAsync = getCoreRoom()->getRoomMgr()->getSvrApp()->getAsynReqQueue();
	Json::Value jsReqLeave;
	jsReqLeave["coin"] = ps->nChips;
	jsReqLeave["targetUID"] = nUserUID;
	pAsync->pushAsyncRequest(ID_MSG_PORT_DATA, nUserUID, eAsync_SyncPlayerGameInfo, jsReqLeave);
}

uint32_t ICoinRoom::getEnterLimitLow()
{
	return m_nEnterLimitLow;
}

uint32_t ICoinRoom::getEnterLimitTop()
{
	return m_nEnterLimitTop;
}

bool ICoinRoom::isDuringGame()
{
	if (getCoreRoom() == nullptr)
	{
		return false;
	}

	auto nStateID = getCoreRoom()->getCurState()->getStateID();
	return nStateID != eRoomSate_WaitReady && nStateID != eRoomState_GameEnd;
}

int32_t ICoinRoom::getDeskFee()
{
	return m_nDeskFee;
}
