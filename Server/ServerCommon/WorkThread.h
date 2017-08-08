#pragma once
#include "ThreadMod.h"
#include "ITask.h"
#include <mutex>
#include <condition_variable>
class CTaskPool ;
class CWorkThread
	:public CThreadT
{
public:
	CWorkThread( CTaskPool* pool,uint8_t nIdx );
	void assignTask(ITask::ITaskPrt pTask );
	ITask::ITaskPrt getTask();
	void __run() ;
	void close();
	uint8_t getIdx() { return m_nIdx; }
protected:
	ITask::ITaskPrt m_pTask ;

	std::mutex m_tMutex;
	std::condition_variable m_tCondition;
	bool m_isHaveNewTask = false;

	bool m_isClose; 
	CTaskPool* m_pPool ;
	uint8_t m_nIdx ;
};