#pragma once
#include "ILeagueComponent.h"
class CLeague;
class CLeagueMemberData
	:public ILeagueComponent
{
public:
	struct stMemberBaseData
	{
		uint32_t nMemberCID;
		uint8_t nState;
		uint8_t nLevel;
		uint32_t nJoinTime;
		uint32_t nQuitTime;
		int32_t nIntegration = 0;
		int32_t nInitialIntegration = 0;
		uint8_t nStop = 0;
		uint32_t nTempIntegration = 0;
	};
	typedef std::map<uint32_t, stMemberBaseData> MAP_CID_MEMBERS;

public:
	CLeagueMemberData();
	~CLeagueMemberData();
	void init(CLeague* pLeague)override;
	void reset()override;
	void timerSave()override;
	bool onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort, uint32_t nSenderID)override;
	bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)override;
	bool addMember(uint32_t nMemberCID, uint8_t nLevel = eLeagueMemberLevel_None);
	bool isNotJoin(uint32_t uMemberCID);
	uint16_t getMemberCnt();
	void memberDataToJson(Json::Value& jsData);
	void memberIDToJson(Json::Value& jsData);
	bool checkUpdateLevel(uint32_t nMemberCID, uint8_t nLevelRequired);
	uint8_t getMemberLevel(uint32_t nMemberCID);
	bool grantIntegration(uint32_t nGrantCID, uint32_t nMemberCID, uint32_t nAmount);
	bool addIntegration(uint32_t nGrantCID, int32_t nAmount);
	bool addInitialIntegration(uint32_t nGrantCID, int32_t nAmount);
	bool setStopState(uint32_t nMemberCID, uint8_t& nState);
	uint8_t getStropState(uint32_t nMemberCID);
	int32_t getIntegration(uint32_t nMemberCID);
	int32_t getInitialIntegration(uint32_t nMemberCID);
	bool checkDecreaseIntegration(uint32_t nMemberCID, uint32_t nAmount);
	bool decreaseIntegration(uint32_t nMemberCID, uint32_t nAmount);
	bool clearTempIntegration(uint32_t nMemberCID, uint32_t nAmount);
	bool fireClub(uint32_t nMemberCID, uint32_t nFireCID);
	bool onClubQuit(uint32_t nMemberCID);
	void pushAsyncRequestToAll(eMsgPort nPortID, eAsyncReq nReqType, Json::Value& jsData);
	void pushAsyncRequestToLevelNeed(eMsgPort nPortID, eAsyncReq nReqType, Json::Value& jsData, uint8_t nLevel = eLeagueMemberLevel_None);

protected:
	void readMemberFormDB(uint32_t nOffset = 0);
	void doProcessAfterReadDB();
	bool findCreator();
	bool isNotDirty(uint32_t nMemberCID);

protected:
	bool m_bReadingDB;
	MAP_CID_MEMBERS m_mMembers;
	std::vector<uint32_t> m_vMemberDertyCIDs;
	std::vector<uint32_t> m_vMemberAddCIDs;

};