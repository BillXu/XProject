#pragma once
#include "IPlayerComponent.h"
#include <set>
#include <list>
class CPlayerGameData
	:public IPlayerComponent
{
public:
	struct stRoomEntry
	{
		uint32_t nRoomID;
		eMsgPort nSvrPort;

		stRoomEntry() { clear(); }
		stRoomEntry(uint32_t ID, eMsgPort nPort) { nRoomID = ID; nSvrPort = nPort; }
		void clear() { nRoomID = 0; nSvrPort = ID_MSG_PORT_MAX; }
		bool isEmpty()const { return nRoomID == 0; }
	};
public:
	CPlayerGameData(CPlayer* pPlayer) :IPlayerComponent(pPlayer) { m_eType = ePlayerComponent_PlayerGameData; m_isWhiteListDirty = false; }
	~CPlayerGameData();
	void reset()override;
	void setStayInRoom( stRoomEntry tRoom );
	const stRoomEntry& getStayInRoom();
	bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)override;
	void onPlayerDisconnect()override;
	void onPlayerReconnected()override;
	void onPlayerLoseConnect()override;  // wait player reconnect ;
	void onPlayerOtherDeviceLogin( uint16_t nOldSessionID, uint16_t nNewSessionID )override;
	bool canRemovePlayer()override;
	bool onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort)override;
	void onPlayerLogined()override;
	bool onOtherSvrShutDown(eMsgPort nSvrPort, uint16_t nSvrIdx, uint16_t nSvrMaxCnt)override;
	void timerSave()override;
	bool isUserIDInWhiteList( uint32_t nUserUID );
	void adminVisitInfo( Json::Value& jsInfo );
protected:
	void informNetState( uint8_t nStateFlag ); //  0 online , 1 wait reconnect , 2 offline .
protected:
	stRoomEntry m_tStayRoom;
	std::vector<stRoomEntry> m_vCreatedRooms;
	std::set<uint32_t> m_vWhiteList;
	bool m_isWhiteListDirty;
};