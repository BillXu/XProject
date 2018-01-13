#pragma once
#include "IGlobalModule.h"
#include "Club.h"
#include <json/json.h>
class CClubManager
	:public IGlobalModule
{
public:
	typedef std::map<uint32_t, CClub*> MAP_ID_CLUBS;

public:
	CClubManager();
	~CClubManager();

	void init(IServerApp* svrApp)override;
	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID)override;
	CClub* getClubByClubID(uint32_t nClubID);
	void update(float fDeta)override;
	void onExit()override;
	bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)override;
	bool onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID, uint16_t nTargetID)override;

protected:
	void onTimeSave()override;
	void addActiveClub(CClub* pNewClub);
	void readClubFormDB(uint8_t nOffset);
	void doProcessAfterReadDB();

protected:
	MAP_ID_CLUBS m_vAllClubs;
};