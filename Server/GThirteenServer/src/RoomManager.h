#pragma once
#include "IGameRoomManager.h"
class RoomManager
	:public IGameRoomManager
{
public:
	IGameRoom* createRoom(uint8_t nGameType)override;
	uint32_t getDiamondNeed(uint8_t nGameType, uint8_t nLevel, ePayRoomCardType payType)override;
	bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)override;

protected:
	void onPlayerCreateRoom(Json::Value& prealMsg, uint32_t nSenderID)override;
	IGameRoom* doPlayerCreateRoom(Json::Value& prealMsg, uint32_t nDiamondNeed, bool isRoomOwnerPay = false);
};
