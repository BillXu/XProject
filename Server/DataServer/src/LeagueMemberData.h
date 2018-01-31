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
	bool checkUpdateLevel(uint32_t nMemberCID, uint8_t nLevelRequired);
	uint8_t getMemberLevel(uint32_t nMemberCID);
	bool grantIntegration(uint32_t nGrantCID, uint32_t nMemberCID, uint32_t nAmount);
	bool fireClub(uint32_t nMemberCID, uint32_t nFireCID);
	bool onClubQuit(uint32_t nMemberCID);
	void pushAsyncRequestToAll(eMsgPort nPortID, eAsyncReq nReqType, Json::Value& jsData);

protected:
	void readMemberFormDB(uint8_t nOffset = 0);
	void doProcessAfterReadDB();
	bool findCreator();

protected:
	bool m_bReadingDB;
	MAP_CID_MEMBERS m_mMembers;
	std::vector<uint32_t> m_vMemberDertyCIDs;
	std::vector<uint32_t> m_vMemberAddCIDs;

};