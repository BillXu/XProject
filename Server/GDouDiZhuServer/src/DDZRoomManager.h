#pragma once
#include "IGameRoomManager.h"
#include "RoomGroup.h"
class DDZRoomManager
	:public IGameRoomManager
{
public:
	void init(IServerApp* svrApp)override;
	void onConnectedSvr(bool isReconnected)override;
	void update(float fDeta)override;
	bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)override;
	bool onPublicMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID)override;
	IGameRoom* getRoomByID(uint32_t nRoomID)override;
	IGameRoom* createRoom(uint8_t nGameType)override;
	uint8_t getDiamondNeed(uint8_t nGameType, uint8_t nLevel,ePayRoomCardType payType )override;
protected:
	RoomGroup m_vCoinRoomGroup[eRoomLevel_Max];
};
