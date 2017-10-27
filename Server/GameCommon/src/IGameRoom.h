#pragma once
#include "NativeTypes.h"
#include "json/json.h"
#include "MessageIdentifer.h"
#include <memory>
#include "ServerCommon.h"
class IGameRoomManager;
struct stMsg;
struct stEnterRoomData;
class IGameRoom
{
public:
	virtual ~IGameRoom(){}
	virtual bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts ) = 0;
	virtual uint8_t checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer ) = 0 ; // 0 can enter , 1 room is full ;
	virtual bool onPlayerEnter(stEnterRoomData* pEnterRoomPlayer) = 0;
	virtual bool isRoomFull() = 0;
	virtual bool doDeleteRoom() = 0; // wanning: invoke by roomMgr ;

	virtual uint32_t getRoomID() = 0;
	virtual uint32_t getSeiralNum() = 0;
	virtual void update(float fDelta) = 0;
	virtual void sendRoomMsg(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nOmitSessionID = 0 ) = 0;
	virtual void sendMsgToPlayer(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nSessionID) = 0;
	virtual bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort,uint32_t nSessionID ) = 0;
	virtual void sendRoomInfo(uint32_t nSessionID) = 0;
	virtual void sendRoomPlayersInfo( uint32_t nSessionID ) = 0 ;
	virtual bool onPlayerNetStateRefreshed(uint32_t nPlayerID, eNetState nState ) = 0;
	virtual bool onPlayerSetNewSessionID(uint32_t nPlayerID, uint32_t nSessinID ) = 0;
	virtual void packRoomInfo( Json::Value& jsRoomInfo ) = 0;
};