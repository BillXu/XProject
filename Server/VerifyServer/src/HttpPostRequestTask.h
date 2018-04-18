#pragma once
#include "ITask.h"
#include "httpRequest.h"
#include "json/json.h"
class HttpPostRequestTask
	:public ITask
	, public CHttpRequestDelegate
{
public:
	HttpPostRequestTask(uint32_t nTaskID);
	uint8_t performTask()override;
	void onHttpCallBack(char* pResultData, size_t nDatalen, void* pUserData, size_t nUserTypeArg)override;
	void setReqString(std::string& strReqURL, Json::Value jsPostData); 
	Json::Value& getResultJson() { return m_jsResult; }
	void setUserData(Json::Value& jsUserData) { m_jsUserData = jsUserData; }
	Json::Value& getUserData() { return m_jsUserData; }
protected:
	CHttpRequest m_tHttpRequest;
	std::string m_strReqURL;
	Json::Value m_jsPostData;
	Json::Value m_jsResult;

	Json::Value m_jsUserData;
};

