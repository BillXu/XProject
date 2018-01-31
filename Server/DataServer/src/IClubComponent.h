#pragma once
#include "ClubDefine.h"
#include <json/json.h>
#include "ServerMessageDefine.h"

class CClub;
class IClubComponent
{
public:
	IClubComponent();
	virtual ~IClubComponent();
	virtual void reset() {}
	virtual void init(CClub* pClub);
	virtual void timerSave() {}
	void sendMsgToClient(Json::Value& jsMsg, uint16_t nMsgType, uint32_t nSessionID);
	eClubComponentType getComponentType() { return m_eType; }
	CClub* getClub() { return m_pClub; }
	virtual bool onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort, uint32_t nSenderID) { return false; }
	virtual bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult) { return false; }
	virtual bool onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID) { return false; }

protected:
	CClub* m_pClub;
	eClubComponentType m_eType;
};
