#pragma once
#include "IGlobalModule.h"
#include "CommonDefine.h"
class MailModule
	:public IGlobalModule
{
public:
	void init(IServerApp* svrApp)override;
	void onConnectedSvr(bool isReconnected)override;
	uint32_t generateMailID();
	void postMail(uint32_t nTargetID, eMailType emailType, Json::Value& jsMailDetail, uint32_t nState );
protected:
	uint32_t m_nMaxMailID;
};