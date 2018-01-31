#pragma once
#include "ILeagueComponent.h"
class CLeagueMemberData;
class CLeagueManager;
class CLeagueEvent;
class CLeague {
public:
	struct stLeagueBaseData
	{
		char cName[MAX_LEN_CLUB_NAME];
		char cHeadiconUrl[MAX_LEN_CLUBICON_URL];
		uint32_t nLeagueID;
		uint32_t nCreatorCID; //æ„¿÷≤øID
		uint32_t nCreateTime;
		uint8_t nState;
		uint16_t nMemberLimit;
		uint8_t nJoinLimit;

		void reset() {

		}
	};
public:
	CLeague();
	~CLeague();
	void init(CLeagueManager* pLeagueManager);
	void reset(); // for reuse the object ;
	bool onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort, uint32_t nSenderID);
	bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult);
	ILeagueComponent* getComponent(eLeagueComponentType eType);
	void onTimerSave();
	void sendMsgToClient(Json::Value& jsMsg, uint16_t nMsgType, uint32_t nSessionID);
	CLeagueManager* getLeagueMgr() { return m_pLeagueManager; }
	CLeagueMemberData* getLeagueMemberData() { return (CLeagueMemberData*)m_vAllComponents[eLeagueComponent_MemberData]; }
	CLeagueEvent* getLeagueEvent() { return (CLeagueEvent*)m_vAllComponents[eLeagueComponent_Event]; }

	void setLeagueID(uint32_t nLeagueID);
	void setCreatorCID(uint32_t nCreatorCID);
	void setName(const char* cName);
	void setIcon(const char* cIcon);
	void setCreateTime(uint32_t nTime);
	void setState(uint8_t nState);
	void setMemberLimit(uint16_t nMemberLimit);
	void setJoinLimit(uint8_t nType);

	uint32_t getLeagueID();
	uint32_t getCreatorCID();
	uint16_t getMemberLimit();
	uint8_t getJoinLimit();
	uint8_t getState();
	void insertIntoDB();

	bool dismissLeague(uint32_t nClubID);

protected:
	stLeagueBaseData m_stBaseData;
	ILeagueComponent* m_vAllComponents[eLeagueComponent_Max];
	CLeagueManager* m_pLeagueManager;

	bool m_bBaseDataDirty; //name, headIcon, state
	bool m_bUseFulDataDirty; //member limit, join limit
};