#include "WorkThread.h"
#include "TaskPool.h"
CWorkThread::CWorkThread( CTaskPool* pool, uint8_t nIdx )
{
	m_isClose = false ;
	m_pTask = nullptr ;
	m_pPool = pool ;
	m_nIdx = nIdx ;
	m_isHaveNewTask = false;
}

void CWorkThread::assignTask(ITask::ITaskPrt pTask )
{
	if ( this->m_isClose )
	{
		return ;
	}

	m_pTask = pTask ;
	if ( m_pTask )
	{
		{
			std::unique_lock<std::mutex> tLock(m_tMutex);
			m_isHaveNewTask = true;
		}
		m_tCondition.notify_all();
	}
}

ITask::ITaskPrt CWorkThread::getTask()
{
	return m_pTask ;
}

void CWorkThread::__run()
{
	while ( true )
	{
		{
			std::unique_lock<std::mutex> tLock(m_tMutex);
			m_tCondition.wait(tLock, [this]() { return m_isHaveNewTask; });
		}

		if ( m_isClose )
		{
			printf("do close thread \n") ;
			m_pTask = nullptr ;
			break;
		}

		if ( m_pTask )
		{
			auto ret = m_pTask->performTask();
			m_pTask->setResultCode(ret) ;
			if ( m_pPool )
			{
				printf("thread idx = %u ok \n",m_nIdx) ;
				m_pPool->onThreadFinishTask(this) ;
			}
			else
			{
				printf("why pool is null , how to tell pool result \n ") ;
			}
		}
		m_isHaveNewTask = false;
	}
}

void CWorkThread::close()
{
	printf("send close cmd \n") ;
	{
		std::unique_lock<std::mutex> tLock(m_tMutex);
		m_isClose = true;
		m_isHaveNewTask = true;
	}

	m_tCondition.notify_all();
}