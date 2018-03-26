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
	void onPlayerOtherDeviceLogin( uint32_t nOldSessionID, uint32_t nNewSessionID )override;
	bool canRemovePlayer()override;
	bool onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort)override;
	void onPlayerLogined()override;
	bool onOtherSvrShutDown(eMsgPort nSvrPort, uint16_t nSvrIdx, uint16_t nSvrMaxCnt)override;
	void timerSave()override;
	bool isUserIDInWhiteList( uint32_t nUserUID );
	void adminVisitInfo( Json::Value& jsInfo );
	void addDraginedRoom(stRoomEntry tRoom);
	void removeDraginedRoom(uint32_t nRoomID, eMsgPort nSvrPort);
protected:
	void informNetState( uint8_t nStateFlag ); //  0 online , 1 wait reconnect , 2 offline .
	void getClubRoomInfo(Json::Value& jsRooms, const std::vector<uint32_t>& vClubs, uint16_t nmsgType, uint32_t nIdx = 0);
protected:
	stRoomEntry m_tStayRoom;
	std::vector<stRoomEntry> m_vCreatedRooms;
	std::vector<stRoomEntry> m_vDraginedRooms;
	std::set<uint32_t> m_vWhiteList;
	bool m_isWhiteListDirty;
};