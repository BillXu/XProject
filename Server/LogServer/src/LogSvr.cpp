#include "LogSvrApp.h"
#include "DBManager.h"
#include "DataBaseThread.h"
#include "DBRequest.h"
#include "ServerMessageDefine.h"
#include "CommonDefine.h"
#include "log4z.h"
#include "ConfigDefine.h"
CLogSvrApp::CLogSvrApp()
{
	m_pDBManager = NULL ;
	m_pDBWorkThread = NULL ;
}

CLogSvrApp::~CLogSvrApp()
{
	if ( m_pDBManager )
	{
		delete m_pDBManager ;
	}

	if ( m_pDBWorkThread )
	{
		delete m_pDBWorkThread ;
	}
}

bool CLogSvrApp::init(Json::Value& jsSvrCfg)
{
	IServerApp::init(jsSvrCfg);

	CSeverConfigMgr stSvrConfigMgr;
	stSvrConfigMgr.LoadFile("../configFile/serverConfig.txt");
	// set up data base thread 
	auto jsDB = jsSvrCfg["gameDB"];
	if (jsDB.isNull())
	{
		LOGFMTE("data base config is null, cant not start server");
		return false;
	}
	m_pDBWorkThread = new CDataBaseThread ;
	m_pDBWorkThread->InitDataBase(jsDB["ip"].asCString(), jsDB["account"].asCString(), jsDB["pwd"].asCString(), jsDB["name"].asCString());
	m_pDBWorkThread->Start();

	// dbManager ;
	m_pDBManager = new CDBManager() ;
	m_pDBManager->Init();

	LOGFMTI("log server Start!");
	return true ;
}
void CLogSvrApp::update(float fDeta)
{
	IServerApp::update(fDeta);
	// process DB Result ;
	CDBRequestQueue::VEC_DBRESULT vResultOut ;
	CDBRequestQueue::SharedDBRequestQueue()->GetAllResult(vResultOut) ;
	CDBRequestQueue::VEC_DBRESULT::iterator iter = vResultOut.begin() ;
	for ( ; iter != vResultOut.end(); ++iter )
	{
		stDBResult* pRet = *iter ;
		m_pDBManager->OnDBResult(pRet) ;
		delete pRet ;
	}
	vResultOut.clear();
}

// net delegate
bool CLogSvrApp::onLogicMsg( stMsg* prealMsg , eMsgPort eSenderPort , uint32_t nSessionID )
{
	if (  IServerApp::onLogicMsg(prealMsg,eSenderPort,nSessionID) )
	{
		return true;
	}

	if ( prealMsg->cSysIdentifer == ID_MSG_VERIFY )
	{
		return true ;
	}

	if ( m_pDBManager )
	{
		m_pDBManager->OnMessage(prealMsg,eSenderPort,nSessionID) ;
	}
	return true ;
}


void CLogSvrApp::onExit()
{
	m_pDBWorkThread->StopWork();
	LOGFMTI("log svr ShutDown!");
}

uint16_t CLogSvrApp::getLocalSvrMsgPortType()
{
	return ID_MSG_PORT_LOG ;
}