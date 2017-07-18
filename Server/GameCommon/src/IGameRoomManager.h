#pragma once
#include "IGlobalModule.h"
#include <json/json.h>
class IGameRoom;
class IGameRoomManager
	:public IGlobalModule
{
public:
	virtual ~IGameRoomManager();
	IGameRoom* getRoomByID(uint32_t nRoomID);
	bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)override;
	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID )override;
	virtual bool onPublicMsg( Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID );
	uint32_t generateRoomID();
	uint32_t generateSieralID();
	uint32_t generateReplayID();
	void update(float fDeta)override;
	virtual IGameRoom* createRoom() = 0;
	void deleteRoom( uint32_t nRoomID );
	void onConnectedSvr(bool isReconnected)override;
	virtual uint32_t getRoomType() = 0;
protected:
	std::map<uint32_t, IGameRoom*> m_vRooms;
	std::vector<uint32_t> m_vWillDeleteRoom;
	uint32_t m_nMaxSieralID = 0 ;
	uint32_t m_nMaxReplayUID = 0 ;
};