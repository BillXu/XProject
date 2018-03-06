#pragma once
#include "IGlobalModule.h"
#include <json/json.h>

class CClub;
class CLeague;
class CLeagueManager
	:public IGlobalModule
{
public:
	typedef std::map<uint32_t, CLeague*> MAP_ID_LEAGUES;

public:
	CLeagueManager();
	~CLeagueManager();
	void init(IServerApp* svrApp)override;
	void onConnectedSvr(bool isReconnected)override;
	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID)override;
	CLeague* getLeagueByLeagueID(uint32_t nLeagueID);
	void update(float fDeta)override;
	void onExit()override;
	void sendMsgToClient(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nSessionID);
	bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)override;
	bool onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID, uint16_t nTargetID)override;
	bool isReadingDB() { return m_bReadingDB; }
	uint32_t generateEventID();

protected:
	void onTimeSave()override;
	void addActiveLeague(CLeague* pNewLeague);
	void readLeagueFormDB(uint32_t nOffset = 0);
	void doProcessAfterReadDB();
	void createLeague(CClub* pClub, const Json::Value& jsReqContent, uint32_t nSenderID);
	uint32_t generateLeagueID();

protected:
	MAP_ID_LEAGUES m_vAllLeagues;
	uint32_t m_nMaxLeagueID;
	uint32_t m_nMaxEventID;

	bool m_bReadingDB;
};