#include "WorkThread.h"
#include "TaskPool.h"
#include "log4z.h"
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
			LOGFMTD("thrad enter idx = %u",getIdx());
			std::unique_lock<std::mutex> tLock(m_tMutex);
			m_tCondition.wait(tLock, [this]() { return m_isHaveNewTask; });
			LOGFMTD(" awake thread idx = %u", getIdx());
			m_isHaveNewTask = false;
		}

		if ( m_isClose )
		{
			LOGFMTE("do close thread \n") ;
			m_pTask = nullptr ;
			break;
		}

		if ( m_pTask )
		{
			auto ret = m_pTask->performTask();
			m_pTask->setResultCode(ret) ;
			if ( m_pPool )
			{
				LOGFMTD("thread idx = %u ok \n", getIdx()) ;
				m_pPool->onThreadFinishTask(this) ;
				LOGFMTD("thread idx = %u do finished \n", getIdx());
			}
			else
			{
				LOGFMTD("why pool is null , how to tell pool result \n ") ;
			}
		}
	}
}

void CWorkThread::close()
{
	LOGFMTD("send close cmd \n") ;
	{
		std::unique_lock<std::mutex> tLock(m_tMutex);
		m_isClose = true;
		m_isHaveNewTask = true;
	}

	m_tCondition.notify_all();
}