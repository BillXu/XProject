#include "Application.h"
#include "log4z.h"
#include <cassert>
#include "json\json.h"
#include <fstream>
bool CustomAssertFunction(bool isfalse, char* description, int line, char*filepath)
{
	if (true == isfalse)
		return false;
	LOGFMTE("需要调试的位置为 %s 中的 %d 行\n", filepath, line);
	if (IDOK == MessageBoxA(0, description, "Assert调试", MB_OKCANCEL))
		return true;
	else return false;
}

CApplication::CApplication(IServerApp* pApp )
{
	//CLogMgr::SharedLogMgr()->SetOutputFile(nullptr);
	m_pApp = pApp ;
}

IServerApp* CApplication::getApp()
{
	return m_pApp ;
}

void CApplication::stopRuning()
{
	m_isRunning = false ;
}

bool& CApplication::isRunning()
{
	return this->m_isRunning ;
}

void CApplication::startApp()
{
	_CrtSetReportMode(_CRT_ASSERT, 0);
	zsummer::log4z::ILog4zManager::GetInstance()->Start() ;

	// read svr config ;
	Json::Reader js;
	Json::Value jsR;
	std::ifstream ss;
	ss.open("../configFile/svrCfg.txt", std::ifstream::in);
	auto bRet = js.parse(ss, jsR);
	ss.close();
	if (!bRet)
	{
		LOGFMTE("read svrCfg failed");
		return;
	}

	auto nRet = m_pApp->init(jsR);
	assert(nRet && "init svr error");
	if ( nRet == false )
	{
		LOGFMTE("svr init error") ;
		m_pApp = nullptr ;
		return ;
	}

	// create console input thread 
	DWORD threadID;
	HANDLE hThread;
	m_isRunning = true ;
	hThread = CreateThread(NULL,0,CApplication::consoleInput,this,0,&threadID); // 创建线程

	// run loop ;
	uint32_t nRunloop = 0 ;
#ifndef _DEBUG
	while (m_pApp && isRunning())
#endif // !1
	{
		++nRunloop ;
		runAppLoop();
		LOGFMTD("try another loop = %u",nRunloop) ;
		Sleep(800);
	}
	zsummer::log4z::ILog4zManager::GetInstance()->Stop();
}

void CApplication::runAppLoop()
{
#ifdef  _DEBUG
	m_pApp->run();
	return;
#endif //  _DEBUG

	__try
	{
		m_pApp->run() ;
	}
	__except(CatchDumpFile::CDumpCatch::UnhandledExceptionFilterEx(GetExceptionInformation()))
	{
		LOGFMTI("try to recover from exception") ;
	}
}

DWORD WINAPI CApplication::consoleInput(LPVOID lpParam)
{
	auto application = ((CApplication*)lpParam);
	char pBuffer[255] ;
	while(application->isRunning())
	{
		memset(pBuffer,0,sizeof(pBuffer)) ;
		scanf_s("%s",pBuffer,sizeof(pBuffer)) ;
		if ( strcmp(pBuffer,"exit") == 0 || strcmp(pBuffer,"Q") == 0 )
		{
			application->stopRuning();
			IServerApp* pAp = application->getApp() ;
			pAp->stop();
			printf("Closing!!!\n");
		}
		else
		{
			printf("Input exit or Q , to close the app \n") ;
		}
	}
	return 0;
}