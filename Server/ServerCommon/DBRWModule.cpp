#include "DBRWModule.h"
#include "DBTask.h"
#include "log4z.h"
bool DBRWModule::init( const char* pIP, const char* pAccount, const char* pPassword, const char* pDBName, uint8_t nWorkThreadCnt, uint16_t nPort )
{
	m_tTaskPool.init(this, nWorkThreadCnt);
	m_strDBIp = pIP;
	m_strAccount = pAccount;
	m_strPwd = pPassword;
	m_strDBName = pDBName;
	m_nWorkThreadCnt = nWorkThreadCnt;
	m_nDBPort = nPort;
	return true;
}

bool DBRWModule::postDBRequest(stDBRequest::s_ptr ptrRequest, stDBRequest::lpDBResultCallBack lpcallBack)
{
	ptrRequest->lpfCallBack = lpcallBack;
	auto pTask = m_tTaskPool.getReuseTaskObjByID(1);
	if (pTask == nullptr)
	{
		if (m_vWaitToProcessDBRequest.size() > 500)
		{
			LOGFMTE("why have so many request not processed ? is db crashed ? ");
		}

		if (m_vWaitToProcessDBRequest.size() > 1000 )
		{
			LOGFMTE("why have so many request not processed ? is db crashed ? can not push more ");
			return true;
		}
		m_vWaitToProcessDBRequest.push_back(ptrRequest);
		return true;
	}
	
	auto pDBTask = std::static_pointer_cast<CDBTask>(pTask);
	pDBTask->setDBRequest(ptrRequest);
	if ( pDBTask->getCallBack() == nullptr )
	{
		pDBTask->setCallBack(std::bind(&DBRWModule::onTaskFinish,this,std::placeholders::_1));
	}

	m_tTaskPool.postTask(pDBTask);
	return true;
}

void DBRWModule::update()
{
	m_tTaskPool.update();
}

stDBRequest::s_ptr DBRWModule::getReserveReqObj()
{
	auto iter = m_vReseverDBRequest.begin();
	if (iter == m_vReseverDBRequest.end())
	{
		std::shared_ptr<stDBRequest> p (new stDBRequest() );
		return p;
	}

	auto p = m_vReseverDBRequest.front();
	m_vReseverDBRequest.pop_front();
	p->reset();
	return p;
}

ITask::ITaskPrt DBRWModule::createTask(uint32_t nTaskID)
{
	if ( m_nAlreadyCreatedTaskObj > m_nWorkThreadCnt * 2 )
	{
		LOGFMTW("already create two much task obj");
		return nullptr;
	}

	++m_nAlreadyCreatedTaskObj;

	auto p = std::make_shared<CDBTask>(nTaskID, m_strDBIp.c_str(), m_nDBPort, m_strAccount.c_str(), m_strPwd.c_str(), m_strDBName.c_str());
	return p;
}

void DBRWModule::onTaskFinish( ITask::ITaskPrt pTask )
{
	auto pDBTask = std::static_pointer_cast<CDBTask>(pTask);
	auto pReq = pDBTask->getDBRequest();
	auto pRet = pDBTask->getDBResult();
	if (pReq->lpfCallBack == nullptr)
	{
		LOGFMTW("db request = %u call back is null , do not mind result ?",pReq->nRequestUID );
	}
	else
	{
		if (pDBTask->getResultCode() == 0 || pReq->nRetryTimes >= 3 ) // maybe need do reconnect , 2020/3/5 14:26
		{
			pReq->lpfCallBack(pReq, pRet);
		}
		else
		{
			LOGFMTE("db request error , try later again");// maybe need do reconnect , 2020/3/5 14:26
		}
	}

	// request obj push to reserver for resue 
	if (pDBTask->getResultCode() == 0 || pReq->nRetryTimes >= 3 ) // maybe need do reconnect , 2020/3/5 14:26
	{
		LOGFMTD("do finish db request = %u then push to reserver for next use", pReq->nRequestUID);
		pReq->reset();
		m_vReseverDBRequest.push_back(pReq);
	}
	else
	{
		m_vWaitToProcessDBRequest.push_back(pReq);
		++pReq->nRetryTimes;
		LOGFMTE( "db request error , try later again ret = %d, times = %d", pDBTask->getResultCode() , pReq->nRetryTimes );// maybe need do reconnect , 2020/3/5 14:26
	}

	// check wait process db request 
	if ( m_vWaitToProcessDBRequest.empty() )
	{
		return;
	}

	auto pTaskGoOn = m_tTaskPool.getReuseTaskObjByID(1);
	if (pTaskGoOn == nullptr)
	{
		LOGFMTE( "why this time ptask is null ? should not be null ,just one task finished" );
		return;
	}
	pTaskGoOn->setCallBack(std::bind(&DBRWModule::onTaskFinish, this, std::placeholders::_1));

	auto pDBTaskGoOn = std::static_pointer_cast<CDBTask>(pTaskGoOn);
	auto pIter = m_vWaitToProcessDBRequest.begin();
	auto ptrRequest = *pIter;
	m_vWaitToProcessDBRequest.erase(pIter);

	pDBTaskGoOn->setDBRequest(ptrRequest);
	m_tTaskPool.postTask(pDBTaskGoOn);
	LOGFMTD( "go on do request " );
}

void DBRWModule::closeAll()
{
	m_tTaskPool.closeAll();
}