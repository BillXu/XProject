#pragma once
#include "IClubComponent.h"
class CClubGameData;
class CClubMemberData;
class CClubManager;
class CClub 
{
public:
	struct stClubBaseData
	{
		char cName[MAX_LEN_CLUB_NAME];
		char cHeadiconUrl[MAX_LEN_CLUBICON_URL];
		char cDescription[MAX_LEN_DESCRIPTION];
		char cRegion[MAX_LEN_REGION];
		uint32_t nClubID;
		uint32_t nCreatorUID;
		uint32_t nCreateTime;
		uint8_t nState;
		uint16_t nMemberLimit;
		uint16_t nTempMemberLimit = 0;
		uint32_t nFoundation; //基金 用于发放
		uint32_t nIntegration; //积分 用于联盟带入
		uint8_t nCreateRoomType;
		uint8_t nSearchLimit;
		uint8_t nCreateFlag = 0;

		std::vector<uint32_t> vJoinedLeague;
		std::vector<uint32_t> vCreatedLeague;

		void eraseJoinedLeague(uint32_t nLeagueID) {
			auto it = std::find(vJoinedLeague.begin(), vJoinedLeague.end(), nLeagueID);
			while (it != vJoinedLeague.end()) {
				vJoinedLeague.erase(it);
				it = std::find(vJoinedLeague.begin(), vJoinedLeague.end(), nLeagueID);
			}
		}

		void eraseCreatedLeague(uint32_t nLeagueID) {
			auto it = std::find(vCreatedLeague.begin(), vCreatedLeague.end(), nLeagueID);
			while (it != vCreatedLeague.end()) {
				vCreatedLeague.erase(it);
				it = std::find(vCreatedLeague.begin(), vCreatedLeague.end(), nLeagueID);
			}
		}

		void eraseLeague(uint32_t nLeagueID) {
			eraseCreatedLeague(nLeagueID);
			eraseJoinedLeague(nLeagueID);
		}

		bool isNotJoinedLeague(uint32_t nLeagueID) {
			return std::find(vJoinedLeague.begin(), vJoinedLeague.end(), nLeagueID) == vJoinedLeague.end();
		}

		bool isNotCreatedLeague(uint32_t nLeagueID) {
			return std::find(vCreatedLeague.begin(), vCreatedLeague.end(), nLeagueID) == vCreatedLeague.end();
		}

		bool isNotJoinOrCreateLeague(uint32_t nLeagueID) {
			return isNotJoinedLeague(nLeagueID) && isNotCreatedLeague(nLeagueID);
		}

		std::string jlToString() {
			std::ostringstream ssSql;
			bool isFirst = true;
			for (auto ref : vJoinedLeague) {
				if (isFirst) {
					isFirst = false;
					ssSql << std::to_string(ref);
				}
				else {
					ssSql << "." << std::to_string(ref);
				}
			}
			return ssSql.str();
		}

		std::string clToString() {
			std::ostringstream ssSql;
			bool isFirst = true;
			for (auto ref : vCreatedLeague) {
				if (isFirst) {
					isFirst = false;
					ssSql << std::to_string(ref);
				}
				else {
					ssSql << "." << std::to_string(ref);
				}
			}
			return ssSql.str();
		}

		void zeroReset() {
			memset(cName, 0, sizeof(cName));
			memset(cHeadiconUrl, 0, sizeof(cHeadiconUrl));
			memset(cDescription, 0, sizeof(cDescription));
			memset(cRegion, 0, sizeof(cRegion));
			nClubID = 0;
			nCreatorUID = 0;
			nCreateTime = 0;
			nState = 0;
			nMemberLimit = DEFAULT_CLUB_MEMBER_LIMIT;
			nFoundation = 0;
			nIntegration = 0;
			nCreateRoomType = eClubCreateRoom_All;
			nSearchLimit = eClubSearchLimit_None;
			vJoinedLeague.clear();
			vCreatedLeague.clear();
		}
	};
public:
	CClub();
	~CClub();
	void init(CClubManager* pClubManager);
	void reset(); // for reuse the object ;
	bool onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort, uint32_t nSenderID);
	bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult);
	bool onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID);
	IClubComponent* getComponent(eClubComponentType eType);
	void onTimerSave();
	//void onRelease();
	//bool canRelease();
	void sendMsgToClient(Json::Value& jsMsg, uint16_t nMsgType, uint32_t nSessionID);
	CClubManager* getClubMgr() { return m_pClubManager; }
	CClubMemberData* getClubMemberData() { return (CClubMemberData*)m_vAllComponents[eClubComponent_MemberData]; }
	CClubGameData* getClubGameData() { return (CClubGameData*)m_vAllComponents[eClubComponent_GameData]; }
	stClubBaseData* getBaseData() { return &m_stBaseData; }

	void setClubID(uint32_t nClubID);
	void setCreatorUID(uint32_t nCreatorID);
	void setName(const char* cName);
	void setIcon(const char* cIcon);
	void setCreateTime(uint32_t nTime);
	void setRegion(const char* cRegion);
	void setState(uint8_t nState);
	void setMemberLimit(uint16_t nMemberLimit);
	void setFoundation(uint32_t nFoundation);
	void setIntegration(uint32_t nIntegration);
	void setCreateRoomType(uint8_t nCreateRoomType);
	void setSearchLimit(uint8_t nSearchLimit);
	void setCreateFlag(uint8_t nCreateFlag);
	void setDescription(const char* cDescription);

	void signUsefulDataDirty() { m_bUseFulDataDirty = true; };

	void addFoundation(int32_t nAmount);
	void addIntegration(int32_t nIntegration);
	void addJoinedLeague(uint32_t nLeagueID);
	void addCreatedLeague(uint32_t nLeagueID);
	void addMemberLimit(uint16_t nTemp);
	void addTempMemberLimit(uint16_t nTemp);

	void clearTempMemberLimit(uint16_t nTemp);

	uint32_t getClubID();
	uint32_t getCreatorUID();
	uint16_t getMemberLimit();
	uint16_t getTempMemberLimit();
	uint32_t getFoundation();
	uint8_t getState();
	uint8_t getCreateRoomType();
	uint32_t getIntegration();
	uint32_t getCreateFlag();
	void insertIntoDB();

	bool dismissClub();
	void readLeagueIntegration(Json::Value jsInfo, std::vector<uint32_t> vLeagues, uint32_t nReqSerial, uint16_t nSenderPort, uint32_t nSenderID, uint32_t nIdx = 0);

protected:
	stClubBaseData m_stBaseData;
	IClubComponent* m_vAllComponents[eClubComponent_Max];
	CClubManager* m_pClubManager;

	bool m_bBaseDataDirty; //name, headIcon, description
	bool m_bMoneyDataDirty; //foundation, integration
	bool m_bLevelInfoDirty; //state, memberLimit
	bool m_bUseFulDataDirty; //create type, search limit, create flag
	bool m_bLeagueDataDirty; //created league, joined league
};