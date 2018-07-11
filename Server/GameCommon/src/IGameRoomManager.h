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
	virtual IGameRoom* createRoom( uint8_t nGameType ) = 0;
	void deleteRoom( uint32_t nRoomID );
	void onConnectedSvr(bool isReconnected)override;
	virtual uint16_t getDiamondNeed(uint8_t nGameType, uint8_t nLevel, ePayRoomCardType payType, uint16_t nSeatCnt ) = 0;  //warnning :  must condiser isCreateRoomFree ;
protected:
	bool isCreateRoomFree();
	bool isCanCreateRoom();
	void onPlayerCreateRoom( Json::Value& prealMsg,uint32_t nSenderID );
	void prepareRoomIDs();

protected:
	std::map<uint32_t, IGameRoom*> m_vRooms;
	std::vector<uint32_t> m_vWillDeleteRoom;
	std::deque<uint32_t> m_vRoomIDs;
	uint32_t m_nMaxSieralID = 0 ;
	uint32_t m_nMaxReplayUID = 0 ;
	bool m_isCreateRoomFree = false;
	bool m_isCanCreateRoom = true;
};