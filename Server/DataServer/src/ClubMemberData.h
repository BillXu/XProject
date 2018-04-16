#pragma once
#include "IClubComponent.h"
class CClub;
class CClubMemberData
	:public IClubComponent {
public:
	struct stMemberBaseData
	{
		uint32_t nMemberUID;
		uint8_t nState;
		uint8_t nLevel;
		uint32_t nJoinTime;
		uint32_t nQuitTime;
		char cName[MAX_LEN_CHARACTER_NAME];
		char cRemark[MAX_LEN_CLUB_NAME];

		stMemberBaseData() {
			memset(cRemark, 0, sizeof(cRemark));
			memset(cName, 0, sizeof(cName));
		}

		void setRemark(const char* remark) {
			sprintf_s(cRemark, "%s", remark);
		}

		void setName(const char* name) {
			sprintf_s(cName, "%s", name);
		}
	};
	typedef std::map<uint32_t, stMemberBaseData> MAP_ID_MEMBERS;

public:
	CClubMemberData();
	~CClubMemberData();
	void init(CClub* pClub)override;
	void reset()override;
	void timerSave()override;
	bool onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort, uint32_t nSenderID)override;
	bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)override;
	bool onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID)override;
	bool addMember(uint32_t nMemberUID, const char* cName, uint8_t nLevel = eClubMemberLevel_None);
	bool isNotJoin(uint32_t uMemberUID);
	uint16_t getMemberCnt();
	void memberDataToJson(Json::Value& jsData);
	bool checkUpdateLevel(uint32_t nMemberID, uint8_t nLevelRequired, bool canEquals = true);
	uint8_t getMemberLevel(uint32_t nMemberID);
	char* getMemberRemark(uint32_t nMemberID);
	bool grantFoundation(uint32_t nGrantUID, uint32_t nMemberUID, uint32_t nAmount);
	void pushAsyncRequestToAll(eMsgPort nPortID, eAsyncReq nReqType, Json::Value& jsData);
	void pushAsyncRequestToLevelNeed(eMsgPort nPortID, eAsyncReq nReqType, Json::Value& jsData, uint8_t nLevel = eClubMemberLevel_None);
	bool fireMember(uint32_t nMemberUID);
	void checkMemberName(uint32_t nMemberUID, const char* cName);

protected:
	void readMemberFormDB(uint32_t nOffset = 0);
	void doProcessAfterReadDB();
	bool findCreator();

protected:
	bool m_bReadingDB;
	std::vector<uint32_t> m_vMemberDertyIDs;
	std::vector<uint32_t> m_vMemberAddIDs;
	MAP_ID_MEMBERS m_mMembers;
};