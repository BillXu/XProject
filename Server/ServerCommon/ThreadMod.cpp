#include "ThreadMod.h"
#include <thread>
CThreadT::CThreadT()
{
}


CThreadT::~CThreadT()
{
}


bool CThreadT::Start()
{
	std::thread tT(&CThreadT::__threadfunc,this);
	tT.detach();
	return true;
}

void CThreadT::Stop()
{
}

