#pragma once
#include "IGameRoomManager.h"
class RoomManager
	:public IGameRoomManager
{
public:
	void onConnectedSvr(bool isReconnected)override;
	bool onPublicMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID)override;
	IGameRoom* createRoom(uint8_t nGameType)override;
	uint8_t getDiamondNeed(uint8_t nGameType, uint8_t nLevel, ePayRoomCardType payType)override;
	IGameRoom* getCoinRoomToEnterByLevel( uint8_t nLevel );
	IGameRoom* createCoinRoom( uint8_t nLevel );
};
