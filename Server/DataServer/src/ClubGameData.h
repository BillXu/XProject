#pragma once
#include "IClubComponent.h"
class CClubGameData
	:public IClubComponent
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
	CClubGameData();
	~CClubGameData();
	void init(CClub* pClub)override;
	void reset()override;
	bool onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort, uint32_t nSenderID)override;
	bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)override;
	bool onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID)override;
	uint16_t getRoomCnt();
	bool isClubCreateThisRoom(uint32_t nRoomID);

protected:
	uint8_t getCreateRoomLevelRequire(uint8_t nCreateType);
	void getLeagueRoomInfo(Json::Value& jsRoomInfo, std::vector<uint32_t> vLeagues, uint32_t nSenderID, uint32_t nIdx = 0);
	void getLeagueRoomInfoByPlayer(Json::Value& jsRoomInfo, std::vector<uint32_t> vLeagues, uint16_t nRequestType, uint32_t nReqSerial, uint16_t nSenderPort, uint32_t nSenderID, uint32_t nIdx = 0);

protected:
	std::vector<stRoomEntry> m_vCreatedRooms;
};