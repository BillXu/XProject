#include "VerifyApp.h"
#include "CommonDefine.h"
#include "log4z.h"
#include "TaskPoolModule.h"
#include "HttpModule.h"
#include "ConfigDefine.h"
bool CVerifyApp::init(Json::Value& jsSvrCfg)
{
	IServerApp::init(jsSvrCfg);
	LOGFMTI("START verify server !") ;
	installModule(eMod_Pool);
	installModule(eMod_Http);

	auto jsHttp = jsSvrCfg["httpSvr"];
	if (jsHttp.isNull())
	{
		LOGFMTE("http svr cfg is null");
		return false;
	}
	m_strWebchatNotifyUrl = jsHttp.asString();

	m_jsDBCfg = jsSvrCfg["gameDB"];
	if (m_jsDBCfg.isNull())
	{
		LOGFMTE("gameDB cfg is null");
		return false;
	}
	return true;
}

Json::Value& CVerifyApp::getDBCfg()
{
	return m_jsDBCfg;
}

const char* CVerifyApp::getWebchatNotifyUrl()
{
	return m_strWebchatNotifyUrl.c_str();
}

uint16_t CVerifyApp::getLocalSvrMsgPortType()
{
	return ID_MSG_PORT_VERIFY ;
}

CTaskPoolModule* CVerifyApp::getTaskPoolModule()
{
	auto p = getModuleByType(eMod_Pool);
	return (CTaskPoolModule*)p;
}

IGlobalModule* CVerifyApp::createModule( uint16_t eModuleType )
{
	auto p = IServerApp::createModule(eModuleType) ;
	if ( p )
	{
		return p ;
	}

	if ( eModuleType == eMod_Pool )
	{
		p = new CTaskPoolModule();
	}
	else if ( eMod_Http == eModuleType )
	{
		p = new CHttpModule();
	}
	return p ;
}