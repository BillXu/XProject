#pragma once
#include "ILeagueComponent.h"
class CLeagueGameData
	:public ILeagueComponent
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
	CLeagueGameData();
	~CLeagueGameData();
	void init(CLeague* pLeague)override;
	void reset()override;
	bool onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort, uint32_t nSenderID)override;
	bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)override;
	uint16_t getRoomCnt();

protected:
	bool isLeagueCreateThisRoom(uint32_t nRoomID);

protected:
	std::vector<stRoomEntry> m_vCreatedRooms;
};