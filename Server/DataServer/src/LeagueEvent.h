#pragma once
#include "ILeagueComponent.h"
class CLeague;
class CLeagueEvent
	:public ILeagueComponent
{
public:
	struct stEventData
	{
		uint32_t nEventID;
		uint32_t nPostTime;
		uint8_t nEventType;
		uint8_t nLevel;
		uint8_t nState;
		uint32_t nDisposerUID;
		Json::Value jsDetail;
	};
	typedef std::map<uint32_t, stEventData> MAP_ID_EVENTS;
	typedef std::vector<uint32_t> VEC_EVENTIDS;

public:
	CLeagueEvent();
	~CLeagueEvent();
	void init(CLeague* pLeague)override;
	void reset()override;
	void timerSave()override;
	bool onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort, uint32_t nSenderID)override;
	bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)override;
	bool onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID)override;
	void joinEventWaitToJson(Json::Value& jsMsg);

protected:
	void readEventFormDB(uint32_t nOffset = 0);
	void doProcessAfterReadDB();
	uint8_t getEventLevel(uint8_t nEventType);
	uint8_t getEventTreatLevel(uint32_t nEventID);
	bool eventIsDirty(uint32_t nEventID);
	uint8_t treatEvent(uint32_t nEventID, uint32_t nClubID, uint32_t nPlayerID, uint8_t nState);
	bool hasApplayJoin(uint32_t nClubID);

protected:
	MAP_ID_EVENTS m_mAllEvents;
	VEC_EVENTIDS m_vDirtyIDs;
	VEC_EVENTIDS m_vAddIDs;

	bool m_bReadingDB;
};