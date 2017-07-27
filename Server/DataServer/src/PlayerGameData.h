#pragma once
#include "IPlayerComponent.h"
#include <set>
#include <list>
class CPlayerGameData
	:public IPlayerComponent
{
public:
	typedef std::set<uint32_t> SET_ROOM_ID ;
public:
	CPlayerGameData(CPlayer* pPlayer):IPlayerComponent(pPlayer){ m_eType = ePlayerComponent_PlayerGameData ; }
	~CPlayerGameData();
	void setStayInRoomID( uint32_t nRoomID );
	uint32_t getStayInRoomID();
	bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)override;
	void onPlayerDisconnect()override;
	void onPlayerReconnected()override;
	void onPlayerLoseConnect()override;  // wait player reconnect ;
	void onPlayerOtherDeviceLogin( uint16_t nOldSessionID, uint16_t nNewSessionID )override;
	uint16_t getGamePortByRoomID( uint32_t nRoomID );
	bool canRemovePlayer()override;
	bool onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort)override;
protected:
	void informNetState( uint8_t nStateFlag ); //  0 online , 1 wait reconnect , 2 offline .
protected:
	uint32_t m_nStayRoomID = 0 ;
	std::vector<uint32_t> m_vCreatedRooms;
};