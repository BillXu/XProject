#include "ClientManager.h"
#include "GateClient.h"
#include "MessageDefine.h"
#include "log4z.h"
#include "CommonDefine.h"
#include "ServerNetwork.h"
#include "GateServer.h"
#include "ServerMessageDefine.h"
#include "ServerNetwork.h"
#include <time.h>
#include "AsyncRequestQuene.h"
#include "GateServer.h"
#include "Utility.h"
#include "AutoBuffer.h"
#define MAX_INCOME_PLAYER 2000
CGateClientMgr::CGateClientMgr()
{
	m_vNetWorkIDGateClientIdx.clear();
	m_vSessionGateClient.clear() ;
	m_vGateClientReserver.clear();
}

CGateClientMgr::~CGateClientMgr()
{
	MAP_SESSIONID_GATE_CLIENT::iterator iterS = m_vSessionGateClient.begin();
	for ( ; iterS != m_vSessionGateClient.end(); ++iterS )
	{
		delete  iterS->second ;
	}
	m_vSessionGateClient.clear() ;

	// just clear ; object deleted in session Gate ;
	m_vNetWorkIDGateClientIdx.clear() ;

	SET_GATE_CLIENT::iterator iter = m_vGateClientReserver.begin() ;
	for ( ; iter != m_vGateClientReserver.end(); ++iter )
	{
		delete *iter ;
	}
	m_vGateClientReserver.clear() ;
}

bool CGateClientMgr::OnMessage( Packet* pData )
{
	if ( CGateServer::SharedGateServer()->isNative() == false)
	{
		Json::Reader jsRader;
		Json::Value jsRoot;
		auto nRet = jsRader.parse(pData->_orgdata, pData->_orgdata + pData->_len, jsRoot);
		if ( !nRet )
		{
			std::string str(pData->_orgdata,pData->_len);
			LOGFMTE("can not parse the js msg : %s",str.c_str());
			return false;
		}
		return onMsg(jsRoot,pData->_connectID );
	}
	// verify identify 
	stMsg* pMsg = (stMsg*)pData->_orgdata ;
	CHECK_MSG_SIZE(stMsg,pData->_len);
	return onMsg(pMsg,pData->_len,pData->_connectID);
}

bool CGateClientMgr::onMsg(stMsg* pMsg, size_t nMsgLen, CONNECT_ID nNetID)
{
	if ( MSG_VERIFY_CLIENT == pMsg->usMsgType)
	{
		auto pGate = getGateClientByNetWorkID(nNetID);
		if (pGate == nullptr)
		{
			LOGFMTE("you do not connect how to send verify msg");
			return true;
		}
		pGate->doVerifyed();

		stMsgVerifyClientRet msg;
		msg.nRet = 0;
		msg.nSessionID = pGate->getSessionID();
		msg.nTargetID = pGate->getSessionID();
		sendMsgToClient(&msg, sizeof(msg), pGate->getNetworkID());
		return true;
	}

	// client reconnect ;
	if (MSG_RECONNECT == pMsg->usMsgType)
	{
		stMsgReconnect* pRet = (stMsgReconnect*)pMsg;
		auto pBeConnectGate = getGateClientBySessionID(pRet->nSessionID);
		bool bReconnectOk = pBeConnectGate != NULL && pBeConnectGate->getBindUID() > 0;
		stGateClient* pCurGate = getGateClientByNetWorkID(nNetID);
		if (pCurGate == nullptr)
		{
			LOGFMTE("do reconnect why cur player is nullptr ?");
			bReconnectOk = false;
		}

		if (bReconnectOk)
		{
			// remove origin 
			removeActiveClientGate(pCurGate);
			removeActiveClientGate(pBeConnectGate);
			pCurGate->onReconnected(pBeConnectGate);
			addClientGate(pCurGate);

			pBeConnectGate->reset();
			addToResever(pBeConnectGate);

			LOGFMTI("MSG¡¡reconnected ! session id = %d", pRet->nSessionID);
			stMsgClientConnectStateChanged msgRet;
			msgRet.nCurState = 0;
			msgRet.nTargetID = pCurGate->getBindUID();
			sprintf_s(msgRet.cIP, sizeof(msgRet.cIP), "%s", pCurGate->getIP());
			CGateServer::SharedGateServer()->sendMsg(&msgRet, sizeof(msgRet), pCurGate->getSessionID());
			LOGFMTD("tell data svr reconnected ok");
		}

		// send msg to client ;
		stMsgReconnectRet msgback;
		msgback.nRet = (bReconnectOk ? 0 : 1);
		CGateServer::SharedGateServer()->sendMsgToClient((char*)&msgback, sizeof(msgback), nNetID);
		return true;
	}

	// transfer to center server 
	stGateClient* pDstClient = getGateClientByNetWorkID(nNetID);
	if (pDstClient == NULL)
	{
		LOGFMTE("can not send message to Center Server , client is NULL or not verified, so close the unknown connect");
		CGateServer::SharedGateServer()->GetNetWorkForClients()->ClosePeerConnection(nNetID);
		return true;
	}

	if (MSG_PLAYER_REGISTER == pMsg->usMsgType)
	{
		//onRegister(pMsg, pDstClient);
		return true;
	}
	else if (MSG_PLAYER_LOGIN == pMsg->usMsgType)
	{
		//onLogin(pMsg, pDstClient);
		return true;
	}

	if (pMsg->nTargetID == 0)
	{
		LOGFMTE("client send msg = %u , port = %d targetid  is null, uid = %u", pMsg->usMsgType, pMsg->cSysIdentifer, pDstClient->getBindUID());
	}

	if (pDstClient->getBindUID() == 0)
	{
		LOGFMTE("player DstClient not bind uid , so can not transfer msg ip = %s session id = %u , msg = %u", pDstClient->getIP(), pDstClient->getSessionID(), pMsg->usMsgType);
		return true;
	}
	CGateServer::SharedGateServer()->sendMsg(pMsg, nMsgLen, pDstClient->getSessionID());
	return true;
}

bool CGateClientMgr::onMsg( Json::Value& jsMsg, CONNECT_ID nNetID )
{
	auto nPort = jsMsg["cSysIdentifer"].asUInt();
	if ( ID_MSG_PORT_GATE != nPort )
	{		
		stMsgJsonContent msg;
		msg.cSysIdentifer = nPort;
		msg.nJsLen = jsMsg["nJsLen"].asUInt();
		msg.nTargetID = jsMsg["nTargetID"].asUInt();
		// transfer to center server 
		stGateClient* pDstClient = getGateClientByNetWorkID(nNetID);
		if (pDstClient == NULL)
		{
			LOGFMTE("can not send message to Center Server , client is NULL or not verified, so close the unknown connect");
			CGateServer::SharedGateServer()->GetNetWorkForClients()->ClosePeerConnection(nNetID);
			return true;
		}

		if ( msg.nTargetID == 0 )
		{
			LOGFMTE("client send msg = %u , port = %d targetid  is null, uid = %u", msg.usMsgType, msg.cSysIdentifer, pDstClient->getBindUID());
		}

		if (pDstClient->getBindUID() == 0)
		{
			LOGFMTE("player DstClient not bind uid , so can not transfer msg ip = %s session id = %u , msg = %u", pDstClient->getIP(), pDstClient->getSessionID(), msg.usMsgType);
			return true;
		}

		auto js = jsMsg["JS"].asString();
		msg.nJsLen = js.size();
		CAutoBuffer msgBuffer(sizeof(msg) + js.size());
		msgBuffer.addContent(&msg, sizeof(msg));
		msgBuffer.addContent(js.c_str(), js.size());

		return CGateServer::SharedGateServer()->sendMsg((stMsg*)msgBuffer.getBufferPtr(), msgBuffer.getContentSize(), pDstClient->getSessionID());
	}

	if ( jsMsg["JS"].isNull() || jsMsg["JS"].isString() == false )
	{
		LOGFMTE( "js key is null " );
		return false;
	}
	auto nGateMsg = jsMsg["JS"].asString();
	Json::Value jsGateMsg;
	Json::Reader jsR;
	auto ret = jsR.parse(nGateMsg, jsGateMsg);
	if (!ret)
	{
		LOGFMTE( "parse gate msg error = %s",nGateMsg.c_str());
		return true;
	}

	auto nmsgType = jsGateMsg[JS_KEY_MSG_TYPE].asUInt();
	switch (nmsgType)
	{
	case MSG_VERIFY_CLIENT:
	{
		auto pGate = getGateClientByNetWorkID(nNetID);
		if (pGate == nullptr)
		{
			LOGFMTE("you do not connect how to send verify msg netID = %u",nNetID );
			return true;
		}
		pGate->doVerifyed();

		Json::Value jsRet;
		jsRet["nRet"] = 0;
		jsRet["nSessionID"] = pGate->getSessionID();
		jsRet["nTargetID"] = pGate->getSessionID();
		sendMsgToClient(jsRet, nmsgType, pGate->getNetworkID() );
	}
	break;
	case MSG_RECONNECT:
	{
		auto nSessionID = jsGateMsg["nSessionID"].asUInt();
		auto pBeConnectGate = getGateClientBySessionID(nSessionID);
		bool bReconnectOk = pBeConnectGate != NULL && pBeConnectGate->getBindUID() > 0;
		stGateClient* pCurGate = getGateClientByNetWorkID(nNetID);
		if (pCurGate == nullptr)
		{
			LOGFMTE("do reconnect why cur player is nullptr ?");
			bReconnectOk = false;
		}

		if (bReconnectOk)
		{
			// remove origin 
			removeActiveClientGate(pCurGate);
			removeActiveClientGate(pBeConnectGate);
			pCurGate->onReconnected(pBeConnectGate);
			addClientGate(pCurGate);

			pBeConnectGate->reset();
			addToResever(pBeConnectGate);

			LOGFMTI("MSG¡¡reconnected ! session id = %d", nSessionID );
			stMsgClientConnectStateChanged msgRet;
			msgRet.nCurState = 0;
			msgRet.nTargetID = pCurGate->getBindUID();
			sprintf_s(msgRet.cIP, sizeof(msgRet.cIP), "%s", pCurGate->getIP());
			CGateServer::SharedGateServer()->sendMsg(&msgRet, sizeof(msgRet), pCurGate->getSessionID());
			LOGFMTD("tell data svr reconnected ok");
		}

		// send msg to client ;
		Json::Value jsRet;
		jsRet["nRet"] = (bReconnectOk ? 0 : 1);
		sendMsgToClient(jsRet,nmsgType, nNetID );
		return true;
	}
	break;
	case MSG_PLAYER_REGISTER:
	{
		stGateClient* pDstClient = getGateClientByNetWorkID(nNetID);
		if ( !pDstClient )
		{
			LOGFMTE( "MSG_PLAYER_REGISTER gate client is null id = %u",nNetID );
			return false;
		}
		onRegister(jsGateMsg, pDstClient);
	}
	break;
	case MSG_PLAYER_LOGIN:
	{
		stGateClient* pDstClient = getGateClientByNetWorkID(nNetID);
		if (!pDstClient)
		{
			LOGFMTE("MSG_PLAYER_LOGIN gate client is null id = %u", nNetID);
			return false ;
		}
		onLogin(jsGateMsg, pDstClient);
	}
	break;
	default:
		LOGFMTE( "unknown msg type = %u",nmsgType );
		break;
	}
	return true;
}

bool CGateClientMgr::OnHeatBeat( CONNECT_ID nNewPeer )
{
	stGateClient* pCurGate = getGateClientByNetWorkID(nNewPeer);
	if (pCurGate == nullptr)
	{
		LOGFMTE("received heat beat , but gate is nullptr");
		return true;
	}
	pCurGate->doRecivedHeatBet();
	return true;
}

void CGateClientMgr::closeAllClient()
{
	LOGFMTI("close all client peers");
	// remove all connecting ;
	auto iter = m_vSessionGateClient.begin() ;
	for ( ; iter != m_vSessionGateClient.end() ;  )
	{
		// tell other server the peer disconnect
		if (iter->second != nullptr && iter->second->getBindUID() )
		{
			stMsgClientConnectStateChanged msgRet;
			msgRet.nCurState = 2;
			msgRet.nTargetID = iter->second->getBindUID();
			CGateServer::SharedGateServer()->sendMsg( &msgRet, sizeof(msgRet), iter->second->getSessionID() );
			auto nConnectID = iter->second->getNetworkID();
			CGateServer::SharedGateServer()->GetNetWorkForClients()->ClosePeerConnection(nConnectID);
		}
		
		removeActiveClientGate(iter->second) ;
		addToResever(iter->second);

		iter = m_vSessionGateClient.begin() ;
	}
}

void CGateClientMgr::onGateCloseCallBack( stGateClient* pGateClient, bool isWaitReconnect )
{
	if ( isWaitReconnect )
	{
		if ( pGateClient->getBindUID() > 0 )
		{
			LOGFMTD("uid = %u start to wait reconnected",pGateClient->getBindUID() );
			stMsgClientConnectStateChanged msgRet;
			msgRet.nCurState = 1;
			msgRet.nTargetID = pGateClient->getBindUID();
			CGateServer::SharedGateServer()->sendMsg(&msgRet, sizeof(msgRet), pGateClient->getSessionID());

			pGateClient->startWaitReconnect();
			return;
		}
	}

	// tell client svr do disconnected ;
	if (pGateClient->getBindUID() > 0)
	{
		stMsgClientConnectStateChanged msgRet;
		msgRet.nCurState = 2;
		msgRet.nTargetID = pGateClient->getBindUID();
		CGateServer::SharedGateServer()->sendMsg(&msgRet, sizeof(msgRet), pGateClient->getSessionID());
	}

	// do close this connection ;
	LOGFMTD("client connection do disconnected netID = %u",pGateClient->getNetworkID() );
	CGateServer::SharedGateServer()->GetNetWorkForClients()->ClosePeerConnection(pGateClient->getNetworkID());
	removeActiveClientGate(pGateClient);
	addToResever(pGateClient);
}

void CGateClientMgr::onPlayerLogout(uint32_t sessionID)
{
	auto pPlayer = getGateClientBySessionID(sessionID);
	if (pPlayer == nullptr)
	{
		LOGFMTE("player is null , logout session id = %u", sessionID);
		return;
	}
	pPlayer->bindUID(0);
	LOGFMTD("player session id = %u do logout",sessionID );
}

void CGateClientMgr::OnNewPeerConnected(CONNECT_ID nNewPeer, ConnectInfo* IpInfo)
{
	char* strIP = "ip null";
	if ( IpInfo )
	{
		strIP = (char*)IpInfo->strAddress;
		LOGFMTD("a peer connected ip = %s ,port = %d netID = %u",IpInfo->strAddress,IpInfo->nPort,nNewPeer ) ;
	}
	else
	{
		LOGFMTD("a peer connected ip = NULL" ) ;
	}
	
	auto pGate = getReserverGateClient();
	if (pGate == nullptr)
	{
		pGate = new stGateClient();
	}

	pGate->init(CGateServer::SharedGateServer()->generateSessionID(), nNewPeer, strIP );
	pGate->setCloseCallBack(std::bind(&CGateClientMgr::onGateCloseCallBack,this,std::placeholders::_1,std::placeholders::_2));
	addClientGate(pGate);
	
	// check pai dui 
	if ( m_vSessionGateClient.size() > MAX_INCOME_PLAYER )
	{
		pGate->delayClose();
		// send msg to tell client svr is full ;
		stMsgGateSvrFull stFull;
		sendMsgToClient(&stFull,sizeof(stFull),pGate->getNetworkID());
		LOGFMTE("gate is full , please try other gate , cnt = %u",m_vSessionGateClient.size());
	}
}

void CGateClientMgr::OnPeerDisconnected( CONNECT_ID nPeerDisconnected, ConnectInfo* IpInfo  )
{
	// client disconnected ;
	stGateClient* pDstClient = getGateClientByNetWorkID(nPeerDisconnected) ;
	if ( pDstClient == nullptr || pDstClient->isWaitingReconnect() )
	{
		LOGFMTW("gate peer is nullptr , why you get disconnect again ? ");
		return;
	}

	LOGFMTD("one gate peer disconnected");
	onGateCloseCallBack(pDstClient,pDstClient->getBindUID() > 0 );
}

void CGateClientMgr::addClientGate(stGateClient* pGateClient )
{
	auto iter = m_vNetWorkIDGateClientIdx.find(pGateClient->getNetworkID() ) ;
	if ( iter != m_vNetWorkIDGateClientIdx.end() )
	{
		LOGFMTE("why this pos already have data client") ;
		removeActiveClientGate(iter->second);
	}

	auto iterS = m_vSessionGateClient.find(pGateClient->getSessionID()) ;
	if ( iterS != m_vSessionGateClient.end() )
	{
		LOGFMTE("why this pos session id = %d had client data",pGateClient->getSessionID()) ;
		removeActiveClientGate(iter->second);
	}

	m_vNetWorkIDGateClientIdx[pGateClient->getNetworkID()] = pGateClient ;
	m_vSessionGateClient[pGateClient->getSessionID()] = pGateClient ;
}

void CGateClientMgr::removeActiveClientGate(stGateClient* pGateClient )
{
	if ( pGateClient == NULL )
	{
		LOGFMTE("why remove a null client ") ;
		return ;
	}

	MAP_NETWORKID_GATE_CLIENT::iterator iterN = m_vNetWorkIDGateClientIdx.find(pGateClient->getNetworkID()) ;
	if ( iterN != m_vNetWorkIDGateClientIdx.end() )
	{
		m_vNetWorkIDGateClientIdx.erase(iterN) ;
	}
	else
	{
		LOGFMTE("can not find net work id = %d to remove ip = %s",pGateClient->getNetworkID(),pGateClient->getIP() ) ;
	}
	
	MAP_SESSIONID_GATE_CLIENT::iterator iterS = m_vSessionGateClient.find( pGateClient->getSessionID() );
	if ( iterS != m_vSessionGateClient.end() )
	{
		m_vSessionGateClient.erase(iterS) ;
	}
	else
	{
		LOGFMTD("can not find session id = %d to remove ip = %s",pGateClient->getSessionID(),pGateClient->getIP() ) ;
	}
}

stGateClient* CGateClientMgr::getReserverGateClient()
{
	stGateClient* pGateClient = NULL ;
	if ( m_vGateClientReserver.empty() == false )
	{
		auto iter = m_vGateClientReserver.begin();
		pGateClient =  *iter;
		m_vGateClientReserver.erase(iter) ;
	}
	return pGateClient ;
}

stGateClient* CGateClientMgr::getGateClientBySessionID( uint32_t nSessionID)
{
	MAP_SESSIONID_GATE_CLIENT::iterator iter = m_vSessionGateClient.find(nSessionID) ;
	if ( iter == m_vSessionGateClient.end() )
		return NULL ;
	return iter->second ;
}

stGateClient* CGateClientMgr::getGateClientByNetWorkID(CONNECT_ID nNetWorkID )
{
	MAP_NETWORKID_GATE_CLIENT::iterator iter = m_vNetWorkIDGateClientIdx.find(nNetWorkID) ;
	if ( iter != m_vNetWorkIDGateClientIdx.end() )
		return iter->second ;
	return NULL ;
}

void CGateClientMgr::addToResever( stGateClient* pClient )
{
	if (pClient == nullptr)
	{
		LOGFMTE("why add a null gate client to reserver");
		return;
	}
	pClient->reset();
	m_vGateClientReserver.insert(m_vGateClientReserver.end(),pClient);
	if (m_vGateClientReserver.size() > 600)
	{
		auto iter = m_vGateClientReserver.begin();
		delete *iter;
		m_vGateClientReserver.erase(iter);
		LOGFMTE("reserve more than 600 , must delte one ");
	}
}

void CGateClientMgr::sendMsgToClient( stMsg* pmsg, uint16_t nLen, CONNECT_ID nNetWorkID )
{
	CGateServer::SharedGateServer()->sendMsgToClient((char*)pmsg, nLen, nNetWorkID);
}

void CGateClientMgr::sendMsgToClient(Json::Value jsMsg, uint16_t nMsgType, CONNECT_ID nNetWorkID)
{
	jsMsg[JS_KEY_MSG_TYPE] = nMsgType;
	Json::StyledWriter jsWrite;
	auto str = jsWrite.write(jsMsg);
	sendMsgToClient((stMsg*)str.c_str(),str.size(),nNetWorkID);
}

void CGateClientMgr::onLogin( Json::Value& jsMsg, stGateClient* pClient )
{
	auto cAccount = jsMsg["cAccount"].asString();
	auto cPassword = jsMsg["cPassword"].asString();
	// must end with \0
	if (cAccount.size() >= MAX_LEN_ACCOUNT || cPassword.size() >= MAX_LEN_PASSWORD)
	{
		LOGFMTE("password or account len is too long ");
		return;
	}
	auto strAccount = checkStringForSql(cAccount.c_str());
	auto strPass = checkStringForSql(cPassword.c_str());
	// new 
	Json::Value jssql;
	char pBuffer[512] = { 0 };
	sprintf_s(pBuffer, "call CheckAccount('%s','%s')", strAccount.c_str(), strPass.c_str());
	std::string str = pBuffer;
	jssql["sql"] = pBuffer;
	auto pReqQueue = CGateServer::SharedGateServer()->getAsynReqQueue();
	auto nClientNetID = pClient->getNetworkID();
	pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, pClient->getSessionID(),eAsync_DB_Select, jssql, [pReqQueue,this, nClientNetID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut ) {
		
		Json::Value jsRet;
		jsRet["nAccountType"] = 0;
		if (isTimeOut)
		{
			jsRet["nRet"] = 1;
			sendMsgToClient(jsRet, MSG_PLAYER_LOGIN, nClientNetID);
			LOGFMTE("time out why register affect row = 0 net id %u ",nClientNetID );
			return;
		}

		uint8_t nRow = retContent["afctRow"].asUInt();
		Json::Value jsData = retContent["data"];

		uint32_t nUserUID = 0;
		if (jsData.size() != 1)
		{
			jsRet["nRet"] = 1;  // account error ; 
			sendMsgToClient(jsRet, MSG_PLAYER_LOGIN, nClientNetID);
			LOGFMTE("why register affect row = 0 ");
			return;
		}

		Json::Value jsRow = jsData[0u];
		jsRet["nRet"] = jsRow["nOutRet"].asUInt();
		jsRet["nAccountType"] = jsRow["nOutRegisterType"].asUInt();
		nUserUID = jsRow["nOutUID"].asUInt();
		LOGFMTD("check accout = %s  ret = %d", jsRow["strAccount"].asCString(), jsRet["nRet"].asUInt());
		sendMsgToClient(jsRet, MSG_PLAYER_LOGIN, nClientNetID);

		// tell data svr this login ;
		if ( 0 == jsRet["nRet"].asUInt() )
		{
			auto pGateClient = getGateClientByNetWorkID(nClientNetID);
			if (pGateClient == nullptr)
			{
				LOGFMTE("register success , but gate peer disconnected ");
				return;
			}

			pGateClient->bindUID(nUserUID);
			Json::Value jsLogin;
			jsLogin["uid"] = nUserUID;
			jsLogin["ip"] = pGateClient->getIP();
			jsLogin["sessionID"] = pGateClient->getSessionID();
			pReqQueue->pushAsyncRequest(ID_MSG_PORT_DATA, nUserUID,eAsync_Player_Logined, jsLogin);
		}
	}, pClient->getSessionID());
}

void CGateClientMgr::onRegister( Json::Value& jsMsg,stGateClient* pClient )
{
	std::string strName = jsMsg["cName"].asString();
	auto cAccount = jsMsg["cAccount"].asString();
	auto cPassword = jsMsg["cPassword"].asString();
	auto cRegisterType = jsMsg["cRegisterType"].asUInt();
	auto nChannel = jsMsg["nChannel"].asUInt();
	if ( cRegisterType == 0 )
	{
		// rand a name and account 
		char cUAccount[MAX_LEN_ACCOUNT] = { 0 };
		char cUName[MAX_LEN_ACCOUNT] = { 0 };
		for (uint8_t nIdx = 0; nIdx < 8; ++nIdx)
		{
			char acc, cName;
			acc = rand() % 50;
			if (acc <= 25)
			{
				acc = 'a' + acc;
			}
			else
			{
				acc = 'A' + (acc - 25);
			}

			cName = rand() % 50;
			if (cName <= 25)
			{
				cName = 'a' + cName;
			}
			else
			{
				cName = 'A' + (cName - 25);
			}
			cUAccount[nIdx] = acc;
			cUName[nIdx] = cName;
		}
		cPassword = "hello";
		cAccount = cUAccount;
		strName = cUName;
	}
	strName = checkStringForSql(strName.c_str());
	cAccount = checkStringForSql(cAccount.c_str());
	cPassword = checkStringForSql(cPassword.c_str());
	// new 
	Json::Value jssql;
	char pBuffer[512] = { 0 };
	sprintf_s(pBuffer,sizeof(pBuffer), "call RegisterAccount('%s','%s','%s',%d,%d,'%s');", strName.c_str(), cAccount.c_str(), cPassword.c_str(), cRegisterType, nChannel, pClient->getIP());
	std::string str = pBuffer;
	jssql["sql"] = pBuffer;
	auto pReqQueue = CGateServer::SharedGateServer()->getAsynReqQueue();
	auto nRegType = cRegisterType;
	auto nClientNetID = pClient->getNetworkID();
	pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, pClient->getSessionID(),eAsync_DB_Select, jssql, [cRegisterType,pReqQueue,this, nClientNetID, nRegType](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut ) {
		Json::Value jsData = retContent["data"];
		
		uint8_t nRet = 0;
		Json::Value jsRet;
		jsRet["cRegisterType"] = cRegisterType;
		do
		{
			if (jsData.size() != 1)
			{
				nRet = 1;
				break;
			}

			Json::Value jsRow = jsData[0u];
			nRet = jsRow["nOutRet"].asUInt();
			if ( nRet != 0)
			{
				LOGFMTD("register failed duplicate account = %s", jsRow["strAccount"].asCString());
				break;
			}

			jsRet["cAccount"] = jsRow["strAccount"];
			jsRet["cPassword"] = jsRow["strPassword"];
			jsRet["nUserID"] = jsRow["nOutUserUID"];
		} while (0);

		// tell client the success register result ;
		jsRet["nRet"] = nRet;
		sendMsgToClient(jsRet, MSG_PLAYER_REGISTER, nClientNetID);
		// tell data svr this login ;
		if ( 0 == nRet )
		{
			auto pGateClient = getGateClientByNetWorkID(nClientNetID);
			if (pGateClient == nullptr)
			{
				LOGFMTE("login success , but gate peer disconnected ");
				return;
			}

			pGateClient->bindUID(jsRet["nUserID"].asUInt());
			Json::Value jsLogin;
			jsLogin["uid"] = jsRet["nUserID"].asUInt();
			jsLogin["sessionID"] = pGateClient->getSessionID();
			jsLogin["ip"] = pGateClient->getIP();
			pReqQueue->pushAsyncRequest(ID_MSG_PORT_DATA, jsRet["nUserID"].asUInt(), eAsync_Player_Logined, jsLogin);
		}

	},pClient->getSessionID());
}