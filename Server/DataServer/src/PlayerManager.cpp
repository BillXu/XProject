#include "PlayerManager.h"
#include "ServerMessageDefine.h"
#include "log4z.h"
#include "Player.h"
#include "CommonDefine.h"
#include "GameServerApp.h"
#include <assert.h>
#include "EventCenter.h"
#include "PlayerBaseData.h"
#include "AutoBuffer.h"
#include "AsyncRequestQuene.h"
void CPlayerBrifDataCacher::stPlayerDataPrifle::recivedData(Json::Value& jsData, IServerApp* pApp )
{
	if ( isContentData() )
	{
		LOGFMTI("already have player data uid = %d , recive twice ",this->nPlayerUID );
		return ;
	}
	jsBrifData = jsData;
	for ( auto& nSub : vBrifeSubscribers)
	{
		pApp->sendMsg(jsBrifData, MSG_REQUEST_PLAYER_DATA, nPlayerUID, nSub.second.nSessionID, ID_MSG_PORT_CLIENT);
	}
	vBrifeSubscribers.clear();

	for (auto& nSub : vDetailSubscribers)
	{
		pApp->sendMsg(jsBrifData, MSG_REQUEST_PLAYER_DATA, nPlayerUID, nSub.second.nSessionID, ID_MSG_PORT_CLIENT);
	}
	vDetailSubscribers.clear();
}

void CPlayerBrifDataCacher::stPlayerDataPrifle::addSubscriber( uint32_t nSessionId , bool isDetail )
{
	if ( isDetail )
	{
		auto iter = vDetailSubscribers.find(nSessionId) ;
		if ( iter == vDetailSubscribers.end() )
		{
			stSubscriber vt ;
			vt.isDetail = isDetail ;
			vt.nSessionID = nSessionId ;
			vDetailSubscribers[nSessionId] = vt ;
		}
	}
	else
	{
		auto iter = vBrifeSubscribers.find(nSessionId) ;
		if ( iter == vBrifeSubscribers.end() )
		{
			stSubscriber vt ;
			vt.isDetail = isDetail ;
			vt.nSessionID = nSessionId ;
			vBrifeSubscribers[nSessionId] = vt ;
		}
	}

}

CPlayerBrifDataCacher::~CPlayerBrifDataCacher()
{
	for ( MAP_ID_DATA::value_type va : m_vDetailData )
	{
		delete va.second ;
		va.second = nullptr ;
	}
	m_vDetailData.clear() ;
}

void CPlayerBrifDataCacher::removePlayerDataCache( uint32_t nUID )
{
	auto iter = m_vDetailData.find(nUID) ;
	if ( iter != m_vDetailData.end() )
	{
		delete iter->second ;
		iter->second = nullptr ;
		m_vDetailData.erase(iter) ;
	}
}

CPlayerBrifDataCacher::stPlayerDataPrifle* CPlayerBrifDataCacher::getBrifData(uint32_t nUID)
{
	auto iter = m_vDetailData.find(nUID);
	if ( iter == m_vDetailData.end())
	{
		return nullptr;
	}
	return iter->second;
}

bool CPlayerBrifDataCacher::sendPlayerDataProfile(uint32_t nReqUID ,bool isDetail , uint32_t nSubscriberSessionID )
{
	auto iter = m_vDetailData.find(nReqUID) ;
	if ( iter != m_vDetailData.end() )
	{
		auto pData = iter->second;
		iter->second->tRequestDataTime = time(nullptr);
		if (pData->isContentData() == false)
		{
			pData->addSubscriber(nSubscriberSessionID, isDetail);
			LOGFMTD("already req uid = %d data , just add session id  = %d to subscrible list", nReqUID, nSubscriberSessionID);
			return true;
		}
		
		// do send the msg ;
		m_pApp->sendMsg(pData->jsBrifData, MSG_REQUEST_PLAYER_DATA, nReqUID, nSubscriberSessionID, ID_MSG_PORT_CLIENT);
		return true;
	}

	// add to details ;
	auto pData = new stPlayerDataPrifle();
	pData->nPlayerUID = nReqUID;
	pData->tRequestDataTime = time(nullptr);
	pData->addSubscriber(nSubscriberSessionID, isDetail);
	m_vDetailData[nReqUID] = pData;
	// current do not have , req from db 
	auto pReqQueue = m_pApp->getAsynReqQueue();
	// new 
	Json::Value jssql;
	char pBuffer[512] = { 0 };
	sprintf_s(pBuffer, "SELECT nickName,nsex,headIcon FROM playerbasedata where userUID = %u", nReqUID );
	std::string str = pBuffer;
	jssql["sql"] = pBuffer;
	pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, nReqUID, nReqUID, eAsync_DB_Select, jssql, [nReqUID,this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData) {
		uint8_t nRow = retContent["afctRow"].asUInt();
		Json::Value jsData = retContent["data"];

		Json::Value jsBrifData;
		if (jsData.size() == 1)
		{
			jsBrifData["uid"] = nReqUID;
			jsBrifData["name"] = jsData["nickName"];
			jsBrifData["headIcon"] = jsData["headIcon"];
			jsBrifData["sex"] = jsData["nsex"];
		}

		auto pDataIter = m_vDetailData.find(nReqUID);
		if ( pDataIter == m_vDetailData.end() || pDataIter->second == nullptr )
		{
			LOGFMTE("recieved data bu obj is null uid = %u",nReqUID);
			return;
		}
		auto pD = pDataIter->second;
		pD->recivedData(jsBrifData, m_pApp );
	});

	LOGFMTD("request player brif data uid = %u",nReqUID );
#ifdef _DEBUG
	if ( m_vDetailData.size() > 5 )
#else
	if (m_vDetailData.size() > 1000)
#endif
	{
		checkState();
	}
	return true ;
}

void CPlayerBrifDataCacher::visitBrifData(Json::Value jsBrifData, CPlayer* pPlayer )
{
	jsBrifData["uid"] = pPlayer->getUserUID();
	jsBrifData["name"] = pPlayer->getBaseData()->getPlayerName();
	jsBrifData["headIcon"] = pPlayer->getBaseData()->getHeadIcon();
	jsBrifData["sex"] = pPlayer->getBaseData()->getSex();
	jsBrifData["ip"] = pPlayer->getIp();
}

void CPlayerBrifDataCacher::checkState()
{
	auto nNow = (uint32_t)time(nullptr);
	std::vector<uint32_t> vWillRemove;
	for ( auto& iter : m_vDetailData)
	{
#ifdef _DEBUG
		if (nNow > ( 60 * 2 + iter.second->tRequestDataTime))
#else
		if (nNow > (60 * 60 * 2 + iter.second->tRequestDataTime))
#endif
		{
			vWillRemove.push_back(nNow);
		}
	}

	for (auto& ref : vWillRemove)
	{
		auto iter = m_vDetailData.find(ref);
		if ( iter != m_vDetailData.end() )
		{
			m_vDetailData.erase(iter);
			LOGFMTE("remove too old detail data uid = %u",ref);
		}
	}
}

// player manager
CPlayerManager::CPlayerManager()
{
	m_vAllActivePlayers.clear();
}

CPlayerManager::~CPlayerManager()
{
	auto iter = m_vAllActivePlayers.begin();
	for ( ; iter != m_vAllActivePlayers.end() ; ++iter )
	{
		iter->second->onPlayerDisconnect();
		delete iter->second ;
		iter->second = nullptr ;
	}
	m_vAllActivePlayers.clear() ;

	auto iter_R = m_vReserverPlayerObj.begin() ;
	for ( ; iter_R != m_vReserverPlayerObj.end(); ++iter_R )
	{
		delete *iter_R;
		*iter_R = nullptr ;
	}
	m_vReserverPlayerObj.clear();
}

void CPlayerManager::init(IServerApp* svrApp)
{
	IGlobalModule::init(svrApp);
	m_tPlayerDataCaher.init(svrApp);
}

void CPlayerManager::onExit()
{
	auto iter = m_vAllActivePlayers.begin();
	for ( ; iter != m_vAllActivePlayers.end() ; ++iter )
	{
		iter->second->onPlayerDisconnect();
	}
	IGlobalModule::onExit();
}

bool CPlayerManager::onMsg( stMsg* pMessage , eMsgPort eSenderPort , uint32_t nSenderUID )
{
	if ( onPublicMsg(pMessage,eSenderPort, nSenderUID) )
	{
		return true ;
	}

	auto pTargetPlayer = getPlayerByUserUID(pMessage->nTargetID);
	if ( pTargetPlayer && pTargetPlayer->onMsg(pMessage,eSenderPort,nSenderUID ) )
	{
		return true  ;
	}
	else
	{
		if (pTargetPlayer == NULL )
		{
			LOGFMTE("can not find session id = %d to process msg id = %d ,from = %d", nSenderUID,pMessage->usMsgType,eSenderPort) ;
		}
		else
		{
			LOGFMTE( "unprocess msg for player uid = %d , msg = %d ,from %d ",pTargetPlayer->getUserUID(),pMessage->usMsgType,eSenderPort ) ;
		}
	}
	return false ;
}

bool CPlayerManager::onMsg( Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID)
{
	auto pTargetPlayer = getPlayerByUserUID(nTargetID);
	if (pTargetPlayer && pTargetPlayer->onMsg(prealMsg, nMsgType, eSenderPort, nSenderID) )
	{
		return true;
	}
	else
	{
		if (pTargetPlayer == NULL)
		{
			LOGFMTE("can not find session id = %d to process msg id = %d ,from = %d", nSenderID, nMsgType, eSenderPort);
		}
		else
		{
			LOGFMTE("unprocess msg for player uid = %d , msg = %d ,from %d ", pTargetPlayer->getUserUID(), nMsgType, eSenderPort);
		}
	}

	if ( nMsgType == MSG_REQUEST_PLAYER_DATA )
	{
		auto nReqUID = prealMsg["nReqID"].asUInt();
		bool isDetail = prealMsg["isDetail"].asUInt() == 1;
		auto pPlayer = getPlayerByUserUID( nReqUID );
		if ( pPlayer )
		{
			Json::Value jsBrifData;
			m_tPlayerDataCaher.visitBrifData(jsBrifData, pPlayer);
			sendMsg(jsBrifData, MSG_REQUEST_PLAYER_DATA, nReqUID, nSenderID, ID_MSG_PORT_CLIENT );
		}
		else
		{
			m_tPlayerDataCaher.sendPlayerDataProfile(nReqUID, isDetail, nSenderID);
		}
		return true;
	}
	return false ;
}

bool CPlayerManager::onPublicMsg( stMsg* prealMsg , eMsgPort eSenderPort , uint32_t nSenderUID )
{
	switch ( prealMsg->usMsgType )
	{
	case MSG_CLIENT_CONNECT_STATE_CHANGED:
	{
		auto pRet = (stMsgClientConnectStateChanged*)prealMsg;
		auto pPlayer = getPlayerByUserUID(pRet->nTargetID);
		if ( !pPlayer )
		{
			LOGFMTE( "player is null uid = %u , can not update net state = %u , ip = %s",pRet->nTargetID,pRet->nCurState,pRet->cIP );
			return true;
		}
		switch ( pRet->nCurState )
		{
		case 0 :  // do reconnected 
		{
			pPlayer->onPlayerReconnected(pRet->cIP);
		}
		break;
		case 1 : // lose connected , do wait reconnect
		{
			pPlayer->onPlayerLoseConnect();
		}
		break;
		case 2:  // do offlline wait reconnect time out 
		{
			pPlayer->onPlayerDisconnect();
		}
		break;
		default:
			LOGFMTE("uid = %u , recieved unknown net state = %u",pRet->nTargetID,pRet->nCurState);
			return true;
		}
	}
	break;
	default:
	return false;
	}

	return true ;
}

bool CPlayerManager::onAsyncRequest( uint16_t nRequestType , const Json::Value& jsReqContent, Json::Value& jsResult )
{
	if ( jsReqContent["playerUID"].isNull() == false )
	{
		auto nPlayerUID = jsReqContent["playerUID"].asUInt();
		auto pPlayer = getPlayerByUserUID(nPlayerUID);
		if ( pPlayer == nullptr )
		{
			jsResult["ret"] = 1; // can not find target player ;
			return false;
		}
		else
		{
			return pPlayer->onAsyncRequest(nRequestType, jsReqContent, jsResult);
		}
	}

	// common requst ;
	switch (nRequestType)
	{
	case eAsync_Player_Logined:
	{
		auto nUID = jsReqContent["uid"].asUInt();
		auto pIP = jsReqContent["ip"].asCString();
		auto nSessionID = jsReqContent["sessionID"].asUInt();
		auto pPlayer = getPlayerByUserUID(nUID);
		if ( pPlayer )
		{
			if ( nSessionID == pPlayer->getSessionID() )
			{
				LOGFMTE("already logined this session , do login again uid = %u",nUID );
				break;
			}
			pPlayer->onPlayerOtherDeviceLogin(nSessionID, pIP );
			break;
		}

		pPlayer = getReserverPlayerObj();
		pPlayer->onPlayerLogined(nSessionID, nUID, pIP);
		addActivePlayer(pPlayer);
	}
	break;
	//case eAsync_ComsumDiamond:
	//	{
	//		uint32_t nUID = jsReqContent["targetUID"].asUInt() ;
	//		uint32_t nDiamond = jsReqContent["diamond"].asUInt();
	//		auto pPlayer = getPlayerByUserUID(nUID);	
	//		jsResult["diamond"] = nDiamond ;
	//		if ( pPlayer == nullptr || pPlayer->getBaseData()->getAllDiamoned() < nDiamond )
	//		{
	//			jsResult["ret"] = 1 ;
	//			LOGFMTW("palyer uid = %u , consum diamond error , cnt = %u",nUID,nDiamond );
	//		}
	//		else
	//		{
	//			pPlayer->getBaseData()->decressMoney(nDiamond,true ) ;
	//			jsResult["ret"] = 0 ;
	//			LOGFMTD("palyer uid = %u , consum diamond success , cnt = %u",nUID,nDiamond );
	//		}
	//	}
	//	break;
	//case eAsync_GiveBackDiamond:
	//	{
	//		uint32_t nUID = jsReqContent["targetUID"].asUInt() ;
	//		uint32_t nDiamond = jsReqContent["diamond"].asUInt();
	//		auto pPlayer = getPlayerByUserUID(nUID);	
	//		if ( pPlayer == nullptr )
	//		{
	//			Json::Value jsContent;
	//			jsContent["targetUID"] = nUID;
	//			jsContent["addCard"] = nDiamond;
	//			jsContent["addCardNo"] = nUID;
	//			Json::StyledWriter jsWrite;
	//			auto str = jsWrite.write(jsContent);
	//			CPlayerMailComponent::PostMailToPlayer(eMailType::eMail_AddRoomCard, str.c_str(), str.size(), nUID);
	//			LOGFMTE("uid = %u not online can not give back diamond = %u, via agent add card mail ",nUID,nDiamond);
	//		}
	//		else
	//		{
	//			pPlayer->getBaseData()->AddMoney(nDiamond,true);
	//			LOGFMTD("give back diamond uid = %u , cnt = %u",nUID,nDiamond);
	//		}
	//	}
	//	break ;
	//case eAsync_AgentAddRoomCard:
	//{
	//	uint32_t nUserUID = jsReqContent["targetUID"].asUInt();
	//	uint32_t nAddCnt = jsReqContent["addCard"].asUInt();
	//	uint32_t nSeailNumber = jsReqContent["addCardNo"].asUInt();
	//	auto pPlayer = getPlayerByUserUID(nUserUID);
	//	if (nullptr == pPlayer)
	//	{
	//		LOGFMTI("player not online agents add card to uid = %u , cnt = %u , addCardNo = %u", nUserUID, nAddCnt, nSeailNumber);
	//		Json::StyledWriter jsWrite;
	//		auto str = jsWrite.write(jsReqContent);
	//		CPlayerMailComponent::PostMailToPlayer(eMailType::eMail_AddRoomCard,str.c_str(),str.size(),nUserUID);
	//	}
	//	else
	//	{
	//		LOGFMTI("player agents add card to uid = %u , cnt = %u , addCardNo = %u",nUserUID,nAddCnt,nSeailNumber);
	//		pPlayer->getBaseData()->AddMoney(nAddCnt, true);
	//		stMsg msgU;
	//		msgU.usMsgType = MSG_PLAYER_UPDATE_MONEY;
	//		pPlayer->getBaseData()->OnMessage(&msgU, ID_MSG_PORT_CLIENT);
	//	}

	//	jsResult = jsReqContent;
	//}
	//break;
	//case eAsync_AgentGetPlayerInfo:
	//{
	//	uint32_t nUserUID = jsReqContent["targetUID"].asUInt();
	//	jsResult["targetUID"] = jsReqContent["targetUID"];
	//	auto pPlayer = getPlayerByUserUID(nUserUID);
	//	if (nullptr == pPlayer)
	//	{
	//		jsResult["isOnline"] = 0;
	//	}
	//	else
	//	{
	//		jsResult["isOnline"] = 1; 
	//		jsResult["name"] = pPlayer->getBaseData()->GetPlayerName();
	//		jsResult["leftCardCnt"] = pPlayer->getBaseData()->GetAllDiamoned();
	//	}
	//}
	//break;
	default:
		return false;
	}
	return true ;
}

void CPlayerManager::update(float fDeta )
{
	IGlobalModule::update(fDeta);
	// process player  delete
	 for ( auto& pp : m_vWillDeletePlayers )
	 {
		 if ( pp == nullptr )
		 {
			 LOGFMTE("why will delete playe is null ? ");
			 continue;
		 }

		 auto iter = m_vAllActivePlayers.find(pp->getUserUID());
		 if (iter != m_vAllActivePlayers.end())
		 {
			 if (iter->second == nullptr)
			 {
				 LOGFMTE("why active player = %u is nullptr ",pp->getUserUID() );
			 }
			 else
			 {
				 if ( m_vReserverPlayerObj.size() > 200 )
				 {
					 delete iter->second;
					 iter->second = nullptr;
				 }
				 else
				 {
					 iter->second->reset();
					 m_vReserverPlayerObj.push_back(iter->second);
				 }
			 }
			 m_vAllActivePlayers.erase(iter);
		 }
	 }
}

CPlayer* CPlayerManager::getPlayerByUserUID( uint32_t nUserUID )
{
	auto iter = m_vAllActivePlayers.begin() ;
	for ( ; iter != m_vAllActivePlayers.end(); ++iter )
	{
		if ( iter->second )
		{
			if ( nUserUID == iter->second->getUserUID() )
			{
				return iter->second ;
			}
		}
	}
	return nullptr ;
}

void CPlayerManager::addActivePlayer(CPlayer*pPlayer)
{
	if ( !pPlayer )
	{
		LOGFMTE("Can not Add NULL player !") ;
		return ;
	}
	auto iter = m_vAllActivePlayers.find( pPlayer->getUserUID() ) ;
	if ( iter != m_vAllActivePlayers.end() )
	{
		LOGFMTE("Player to add had existed in active map ! , player UID = %d",pPlayer->getUserUID() ) ;
		delete iter->second;
		iter->second = NULL ;
		m_vAllActivePlayers.erase(iter);
	}
	m_vAllActivePlayers[pPlayer->getUserUID()] = pPlayer;

	// process brifdata request ;
	auto p = m_tPlayerDataCaher.getBrifData(pPlayer->getUserUID());
	if ( p && p->isContentData() == false)
	{
		Json::Value jsData;
		m_tPlayerDataCaher.visitBrifData(jsData, pPlayer);
		p->recivedData(jsData, getSvrApp());
	}
	m_tPlayerDataCaher.removePlayerDataCache(pPlayer->getUserUID());
}

void CPlayerManager::logState()
{
	LOGFMTI( "active Player: %d" ,m_vAllActivePlayers.size() ) ;
}

CPlayer* CPlayerManager::getReserverPlayerObj()
{
	CPlayer* pRet = nullptr;
	if ( m_vReserverPlayerObj.empty())
	{
		pRet = new CPlayer();
		pRet->init(this);
	}
	else
	{
		auto iter = m_vReserverPlayerObj.begin();
		pRet = *iter;
		m_vReserverPlayerObj.erase(iter);
	}
	pRet->reset();
	return pRet;
}

void CPlayerManager::doRemovePlayer( CPlayer* pOfflinePlayer )
{
	m_vWillDeletePlayers.push_back(pOfflinePlayer);
}

void CPlayerManager::onTimeSave()
{
	for ( auto& ref : m_vAllActivePlayers )
	{
		if ( ref.second )
		{
			ref.second->onTimerSave();
		}
	}
}

bool CPlayerManager::onOtherSvrShutDown(eMsgPort nSvrPort, uint16_t nSvrIdx, uint16_t nSvrMaxCnt)
{
	if ( IGlobalModule::onOtherSvrShutDown(nSvrPort, nSvrIdx, nSvrMaxCnt) )
	{
		return true;
	}

	if ( ID_MSG_PORT_GATE == nSvrPort )
	{
		LOGFMTE( "gate svr idx = %u max cnt = %u crashed , check player disconnect",nSvrIdx,nSvrMaxCnt );
		if ( 0 == nSvrMaxCnt)
		{
			return false;
		}

		for (auto& ref : m_vAllActivePlayers)
		{
			if ( ref.second->getSessionID() % nSvrMaxCnt == nSvrIdx)
			{
#ifdef _DEBUG
				LOGFMTW("gate crash player uid = %u delay remove",ref.second->getUserUID());
#endif
				ref.second->onPlayerLoseConnect();
				ref.second->delayRemove();  // can not direct disconnect , avoid client login from other gate ;
			}
		}
		return true;
	}
	return false;
}