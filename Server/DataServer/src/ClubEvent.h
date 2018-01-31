#pragma once
#include "IClubComponent.h"
class CClub;
class CClubEvent
	: public IClubComponent
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
	CClubEvent();
	~CClubEvent();
	void init(CClub* pClub)override;
	void reset()override;
	void timerSave()override;
	bool onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort, uint32_t nSenderID)override;
	bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)override;

protected:
	void readEventFormDB(uint8_t nOffset = 0);
	void doProcessAfterReadDB();
	uint8_t getEventLevel(uint8_t nEventType);
	bool eventIsDirty(uint32_t nEventID);
	uint8_t treatEvent(uint32_t nEventID, uint32_t nPlayerID, uint8_t nState, uint32_t nSenderID);
	bool hasApplayJoin(uint32_t nUserID);

protected:
	MAP_ID_EVENTS m_mAllEvents;
	VEC_EVENTIDS m_vDirtyIDs;
	VEC_EVENTIDS m_vAddIDs;

	bool m_bReadingDB;
};