#include "PushMessageTask.h"
#include "log4z.h"
#include <json/json.h>
CPushMessageTask::CPushMessageTask(uint32_t nTaskID)
	:ITask(nTaskID)
{
	LOGFMTI("apple verify sandbox ok");
	m_tHttpRequest.init("https://jl.youhoox.com/index.php");
	m_tHttpRequest.setDelegate(this);
}

uint8_t CPushMessageTask::performTask() {
	// processed this requested ;
	Json::FastWriter jWriter;
	Json::Value jRootValue;

	jRootValue["ct"] = "notify";
	jRootValue["ac"] = "jpush";

	std::string strFinal = jWriter.write(jRootValue);
	auto ret = m_tHttpRequest.performRequest(nullptr, strFinal.c_str(), strFinal.size(), nullptr);
	if (ret)
	{
		return 0;
	}
	return 1;
}

void CPushMessageTask::setInfo(MESSAGE_PUSH_ptr prequst)
{
	m_ptrCurRequest = prequst;
}