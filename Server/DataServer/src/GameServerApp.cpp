#include "GameServerApp.h"
#include "PlayerManager.h"
#include <ctime>
#include "PlayerMail.h"
#include "EventCenter.h"
#include "log4z.h"
#include "ServerStringTable.h"
#include "Player.h"
#include "PlayerGameData.h"

#ifndef USHORT_MAX
#define USHORT_MAX 65535 
#endif
#include <cassert>
DataServerApp::~DataServerApp()
{
	delete m_pConfigManager ;
	m_pConfigManager = nullptr;
}

bool DataServerApp::init()
{
	IServerApp::init();
	srand((unsigned int)time(0));

	CSeverConfigMgr SvrConfigMgr ;
	SvrConfigMgr.LoadFile("../configFile/serverConfig.txt");
	stServerConfig* pConfig = SvrConfigMgr.GetServerConfig(ID_MSG_PORT_CENTER) ;
	if ( pConfig == NULL )
	{
		LOGFMTE("center svr config is null , so can not connected to !") ;
		return false;
	}
	setConnectServerConfig(pConfig);

	CServerStringTable::getInstance()->LoadFile("../configFile/stringTable.txt");

	m_pConfigManager = new CConfigManager ;
	m_pConfigManager->LoadAllConfigFile("../configFile/") ;

	// install module
	for ( uint16_t nModule = eMod_None ; nModule < eMod_Max ; ++nModule )
	{
		auto b = installModule(nModule);
		assert(b && "install this module failed " );
		if ( !b )
		{
			LOGFMTE("install module = %u failed",nModule ) ;
		}
	}
	return true ;
}

CPlayerManager* DataServerApp::getPlayerMgr()
{
	auto p = (CPlayerManager*)getModuleByType(eMod_PlayerMgr) ;
	return p ;
}

bool DataServerApp::onLogicMsg( stMsg* prealMsg , eMsgPort eSenderPort , uint32_t nSenderID)
{
	if ( IServerApp::onLogicMsg(prealMsg,eSenderPort, nSenderID ) )
	{
		return true ;
	}

	if ( processPublicMsg(prealMsg,eSenderPort, nSenderID) )
	{
		return true ;
	}
	LOGFMTE("unprocess msg = %d , from port = %d , nsssionid = %d",prealMsg->usMsgType,eSenderPort, nSenderID) ;
	return true ;
}

bool DataServerApp::onLogicMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID)
{
	if ( IServerApp::onLogicMsg(recvValue,nmsgType,eSenderPort, nSenderID, nTargetID ) )
	{
		return true ;
	}
	return false ;
}

bool DataServerApp::onAsyncRequest(uint16_t nRequestType , const Json::Value& jsReqContent, Json::Value& jsResult )
{
	if ( IServerApp::onAsyncRequest(nRequestType,jsReqContent,jsResult) )
	{
		return true ;
	}
	switch ( nRequestType )
	{
	case eAsync_PostDlgNotice:
		{
			eNoticeType eType = (eNoticeType)jsReqContent["dlgType"].asUInt() ;
			uint32_t nTargetUID = jsReqContent["targetUID"].asUInt() ;
			auto jsArg = jsReqContent["arg"];
			CPlayerMailComponent::PostDlgNotice(eType,jsArg,nTargetUID) ;
			LOGFMTD("do async post dlg notice etype = %u ,targetUID = %u",eType,nTargetUID) ;
		}
		break ;
	default:
		return false ;
	}
	return true ;
}

bool DataServerApp::processPublicMsg( stMsg* prealMsg , eMsgPort eSenderPort , uint32_t nSessionID )
{
	return false ;
}

IGlobalModule* DataServerApp::createModule( uint16_t eModuleType )
{
	IGlobalModule* pMod = IServerApp::createModule(eModuleType) ;
	if ( pMod )
	{
		return pMod ;
	}

	switch (eModuleType)
	{
	case eMod_PlayerMgr:
		{
			pMod = new CPlayerManager() ;
		}
		break;
	default:
		break;
	}
	return pMod ;
}