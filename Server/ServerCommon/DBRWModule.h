#pragma once
#include "NativeTypes.h"
#include <functional>
#include <memory>
#include "TaskPool.h"
#include "DBRequest.h"
class DBRWModule
	: public ITaskFactory
{
public:
	bool init( uint32_t nTaskID, const char* pIP, const char* pAccount, const char* pPassword, const char* pDBName , uint8_t nWorkThreadCnt = 3, uint16_t pPort = 3306 );
	bool postDBRequest( stDBRequest::s_ptr ptrRequest, stDBRequest::lpDBResultCallBack lpcallBack );
	void update();
	stDBRequest::s_ptr getReserveReqObj();
	void onTaskFinish(ITask::ITaskPrt pTask);
protected:
	ITask::ITaskPrt createTask(uint32_t nTaskID)override;
protected:
	CTaskPool m_tTaskPool;
	uint8_t m_nWorkThreadCnt = 0 ;
	uint8_t m_nAlreadyCreatedTaskObj = 0 ;

	std::list<stDBRequest::s_ptr> m_vReseverDBRequest;
	std::list<stDBRequest::s_ptr> m_vWaitToProcessDBRequest;

	std::string m_strDBName;
	std::string m_strDBIp;
	std::string m_strAccount;
	std::string m_strPwd;
	uint16_t m_nDBPort;
};