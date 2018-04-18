#include "HttpPostRequestTask.h"
#include <cassert>
#include "log4z.h"
HttpPostRequestTask::HttpPostRequestTask(uint32_t nTaskID)
	:ITask(nTaskID)
{
	m_tHttpRequest.init("");
	m_tHttpRequest.setDelegate(this);
}

uint8_t HttpPostRequestTask::performTask()
{
	if (m_strReqURL.empty())
	{
		return 1;
	}

	Json::StyledWriter jsw;
	auto str = jsw.write(m_jsPostData);
	auto ret = m_tHttpRequest.performRequest(m_strReqURL.c_str(), str.c_str(), str.size(), nullptr);
	if (ret)
	{
		return 0;
	}
	return 1;
}


void HttpPostRequestTask::setReqString(std::string& strReq, Json::Value jsPostData)
{
	m_strReqURL = strReq;
	m_jsPostData = jsPostData;
}

void HttpPostRequestTask::onHttpCallBack(char* pResultData, size_t nDatalen, void* pUserData, size_t nUserTypeArg)
{
	assert(pResultData != nullptr && "must not null");
	Json::Reader reader;
	m_jsResult.clear();
	if (reader.parse(pResultData, pResultData + nDatalen, m_jsResult))
	{

	}
	else
	{
		std::string str( pResultData,nDatalen);
		LOGFMTE("parse json data error , for post request url = %s content : %s",m_strReqURL.c_str(), str.c_str());
		return;
	}
	return;
}