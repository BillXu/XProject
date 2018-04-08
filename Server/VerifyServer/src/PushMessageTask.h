#pragma once
#include "httpRequest.h"
#include "ITask.h"
struct stPushMessageRequest;
class CPushMessageTask
	:public ITask
	, public CHttpRequestDelegate
{
public:
	typedef std::shared_ptr<stPushMessageRequest> MESSAGE_PUSH_ptr;
public:
	CPushMessageTask(uint32_t nTaskID);
	uint8_t performTask()override;
	void onHttpCallBack(char* pResultData, size_t nDatalen, void* pUserData, size_t nUserTypeArg)override {};
	void setInfo(MESSAGE_PUSH_ptr prequst);
	MESSAGE_PUSH_ptr getCurRequest() { return m_ptrCurRequest; }

protected:
	CHttpRequest m_tHttpRequest;
	MESSAGE_PUSH_ptr m_ptrCurRequest;
};