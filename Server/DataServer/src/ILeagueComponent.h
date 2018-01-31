#pragma once
#include "ClubDefine.h"
#include <json/json.h>
#include "ServerMessageDefine.h"

class CLeague;
class ILeagueComponent
{
public:
	ILeagueComponent();
	virtual ~ILeagueComponent();
	virtual void reset() {}
	virtual void init(CLeague* pLeague);
	virtual void timerSave() {}
	void sendMsgToClient(Json::Value& jsMsg, uint16_t nMsgType, uint32_t nSessionID);
	eLeagueComponentType getComponentType() { return m_eType; }
	CLeague* getLeague() { return m_pLeague; }
	virtual bool onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort, uint32_t nSenderID) { return false; }
	virtual bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult) { return false; }

protected:
	CLeague* m_pLeague;
	eLeagueComponentType m_eType;
};