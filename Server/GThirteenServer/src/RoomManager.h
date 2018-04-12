#pragma once
#include "IGameRoomManager.h"
class RoomManager
	:public IGameRoomManager
{
public:
	IGameRoom* createRoom(uint8_t nGameType)override;
	IGameRoom* createGRoom(uint8_t nGameType);
	IGameRoom* createWRoom(uint8_t nGameType);
	uint32_t getDiamondNeed(uint8_t nGameType, uint8_t nLevel, ePayRoomCardType payType)override;
	bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)override;
	bool onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID, uint16_t nTargetID)override;
	bool onPublicMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID)override;
	void onExit()override;

protected:
	void onPlayerCreateRoom(Json::Value& prealMsg, uint32_t nSenderID)override;
	IGameRoom* doPlayerCreateRoom(Json::Value& prealMsg, uint32_t nDiamondNeed, bool isRoomOwnerPay = false);
	bool isGRoom(uint8_t nLevel);
};
