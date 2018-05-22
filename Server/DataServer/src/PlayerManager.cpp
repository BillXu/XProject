#include "PlayerManager.h"
#include "ServerMessageDefine.h"
#include "log4z.h"
#include "Player.h"
#include "CommonDefine.h"
#include "DataServerApp.h"
#include <assert.h>
#include "EventCenter.h"
#include "PlayerBaseData.h"
#include "AutoBuffer.h"
#include "AsyncRequestQuene.h"
#include <ctime>
#include "MailModule.h"
#include "PlayerGameData.h"
#define Emoji_Golden_Consume 50
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
	sprintf_s(pBuffer, "SELECT nickName,sex,headIcon,allGame,winGame FROM playerbasedata where userUID = %u", nReqUID );
	std::string str = pBuffer;
	jssql["sql"] = pBuffer;
	pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, nReqUID,eAsync_DB_Select, jssql, [nReqUID,this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut ) {
		if (isTimeOut)
		{
			return;
		}

		uint8_t nRow = retContent["afctRow"].asUInt();
		Json::Value jsData = retContent["data"];

		Json::Value jsBrifData;
		if (jsData.size() == 1)
		{
			auto jsRow = jsData[(uint32_t)0];
			jsBrifData["uid"] = nReqUID;
			jsBrifData["name"] = jsRow["nickName"];
			jsBrifData["headIcon"] = jsRow["headIcon"];
			jsBrifData["sex"] = jsRow["sex"];
			jsBrifData["allGame"] = jsRow["allGame"];
			jsBrifData["winGame"] = jsRow["winGame"];
		}
		else
		{
			jsBrifData["ret"] = 1;
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

void CPlayerBrifDataCacher::visitBrifData(Json::Value& jsBrifData, CPlayer* pPlayer )
{
	jsBrifData["uid"] = pPlayer->getUserUID();
	jsBrifData["name"] = pPlayer->getBaseData()->getPlayerName();
	jsBrifData["headIcon"] = pPlayer->getBaseData()->getHeadIcon();
	jsBrifData["sex"] = pPlayer->getBaseData()->getSex();
	jsBrifData["ip"] = pPlayer->getIp();
	jsBrifData["J"] = pPlayer->getBaseData()->getGPS_J();
	jsBrifData["W"] = pPlayer->getBaseData()->getGPS_W();
	jsBrifData["allGame"] = pPlayer->getBaseData()->getAllGame();
	jsBrifData["winGame"] = pPlayer->getBaseData()->getWinGame();
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
	else if ( MSG_PLAYER_LOGOUT == nMsgType)
	{
		if ( pTargetPlayer == nullptr)
		{
			Json::Value js;
			js["ret"] = 1;
			sendMsg(js, nMsgType, nTargetID, nSenderID, ID_MSG_PORT_CLIENT);
			return true;
		}

		if (pTargetPlayer->canRemovePlayer() == false)
		{
			Json::Value js;
			js["ret"] = 2;
			sendMsg(js, nMsgType, nTargetID, nSenderID, ID_MSG_PORT_CLIENT);
			return true;
		}
		pTargetPlayer->onPlayerDisconnect();
		LOGFMTD( "target id = %u do logout , process as disconnect , inform gate svr session id = %u",nTargetID,nSenderID );

		auto pAsync = getSvrApp()->getAsynReqQueue();
		Json::Value jsReq;
		jsReq["sessionID"] = nSenderID;
		pAsync->pushAsyncRequest(ID_MSG_PORT_GATE,nSenderID, eAsync_InformGate_PlayerLogout,jsReq);
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

		if (pPlayer->getSessionID() != nSenderUID)
		{
			LOGFMTD( "old gate client delay disconnect uid = %u",pRet->nTargetID  );
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
		LOGFMTD( "uid = %u net state changed to %u", pPlayer->getUserUID(), pRet->nCurState);
	}
	break;
	default:
	return false;
	}

	return true ;
}

bool CPlayerManager::onAsyncRequest( uint16_t nRequestType , const Json::Value& jsReqContent, Json::Value& jsResult )
{
	// common requst ;
	switch (nRequestType)
	{
	case eAsync_thirteen_MTT_Request_PushMsg:
	{
		auto nUserUID = jsReqContent["targetUID"].asUInt();
		auto pPlayer = getPlayerByUserUID(nUserUID);
		if (pPlayer) {
			Json::Value jsMsg = jsReqContent;
			uint16_t nMsgType = jsReqContent["msgType"].asUInt();
			pPlayer->sendMsgToClient(jsMsg, nMsgType);
		}
	}
	break;
	case eAsync_player_DragInRoom_Closed:
	{
		auto nUserUID = jsReqContent["targetUID"].asUInt();
		auto pPlayer = getPlayerByUserUID(nUserUID);
		if (pPlayer) {
			auto gd = (CPlayerGameData*)pPlayer->getComponent(ePlayerComponent_PlayerGameData);
			uint32_t nRoomID = jsReqContent["roomID"].asUInt();
			auto nPort = jsReqContent["port"].asUInt();
			gd->removeDraginedRoom(nRoomID, (eMsgPort)nPort);
		}
	}
	break;
	case eAsync_club_Update_Member_Limit_check_Diamond:
	case eAsync_thirteen_delay_check_Diamond:
	case eAsync_thirteen_reput_check_Diamond:
	{
		auto nUserUID = jsReqContent["targetUID"].asUInt();
		auto pPlayer = getPlayerByUserUID(nUserUID);
		if (pPlayer && pPlayer->isPlayerReady()) {
			uint32_t nDiamond = jsReqContent["diamond"].asUInt();
			if (pPlayer->getBaseData()->getDiamoned() < nDiamond) {
				jsResult["ret"] = 1;
			}
			else {
				jsResult["ret"] = 0;
			}
		}
		else {
			jsResult["ret"] = 2;
		}
	}
	break;
	case eAsync_club_Treat_Event_Message:
	{
		auto nUserUID = jsReqContent["targetUID"].asUInt();
		auto pPlayer = getPlayerByUserUID(nUserUID);
		if (pPlayer && pPlayer->isPlayerReady()) {
			Json::Value jsMsg;
			jsMsg["ret"] = jsReqContent["ret"];
			jsMsg["eventID"] = jsReqContent["eventID"];
			pPlayer->sendMsgToClient(jsMsg, MSG_CLUB_EVENT_APPLY_TREAT);
		}
	}
	break;
	case eAsync_player_club_decline_DragIn:
	{
		auto nUserUID = jsReqContent["targetUID"].asUInt();
		Json::Value jsMailArg;
		jsMailArg["roomID"] = jsReqContent["roomID"];
		jsMailArg["uid"] = jsReqContent["uid"];
		jsMailArg["clubID"] = jsReqContent["clubID"];

		auto pMailModule = ((DataServerApp*)getSvrApp())->getMailModule();
		pMailModule->postMail(nUserUID, eMail_Club_DeclineDragIn, jsMailArg, eMailState_SysProcessed);
	}
	break;
	case eAsync_player_do_Rot_Banker:
	{
		auto nUserUID = jsReqContent["targetUID"].asUInt();
		int32_t nAmount = -1 * jsReqContent["baseScore"].asInt() * 5;
		Json::Value jsMailArg;
		jsMailArg["amount"] = nAmount;
		jsMailArg["roomID"] = jsReqContent["roomID"];

		auto pMailModule = ((DataServerApp*)getSvrApp())->getMailModule();
		pMailModule->postMail(nUserUID, eMail_Room_RotBanker, jsMailArg, eMailState_WaitSysAct);
	}
	break;
	case eAsync_player_apply_Rot_Banker:
	{
		auto nUserUID = jsReqContent["targetUID"].asUInt();
		auto pPlayer = getPlayerByUserUID(nUserUID);
		if (pPlayer && pPlayer->isPlayerReady()) {
			uint32_t nAmount = jsReqContent["baseScore"].asUInt() * 5;
			if (pPlayer->getBaseData()->getCoin() < nAmount) {
				jsResult["ret"] = 1;
			}
			else {
				jsResult["ret"] = 0;
			}
		}
		else {
			jsResult["ret"] = 2;
		}
	}
	break;
	case eAsync_player_do_Show_Cards:
	{
		auto nUserUID = jsReqContent["targetUID"].asUInt();
		int32_t nAmount = -1 * jsReqContent["baseScore"].asInt() * 100;
		Json::Value jsMailArg;
		jsMailArg["amount"] = nAmount;
		jsMailArg["roomID"] = jsReqContent["roomID"];

		auto pMailModule = ((DataServerApp*)getSvrApp())->getMailModule();
		pMailModule->postMail(nUserUID, eMail_Room_ShowCards, jsMailArg, eMailState_WaitSysAct);
	}
	break;
	case eAsync_player_apply_Show_Cards:
	{
		auto nUserUID = jsReqContent["targetUID"].asUInt();
		auto pPlayer = getPlayerByUserUID(nUserUID);
		if (pPlayer && pPlayer->isPlayerReady()) {
			uint32_t nAmount = jsReqContent["baseScore"].asUInt() * 100;
			if (pPlayer->getBaseData()->getCoin() < nAmount) {
				jsResult["ret"] = 1;
			}
			else {
				jsResult["ret"] = 0;
			}
		}
		else {
			jsResult["ret"] = 2;
		}
	}
	break;
	case eAsync_player_clubRoom_Back_Chip:
	{
		auto nUserUID = jsReqContent["targetUID"].asUInt();
		Json::Value jsMailArg;
		jsMailArg["amount"] = jsReqContent["chip"];
		jsMailArg["clubID"] = jsReqContent["clubID"];
		jsMailArg["roomID"] = jsReqContent["roomID"];

		auto pMailModule = ((DataServerApp*)getSvrApp())->getMailModule();
		pMailModule->postMail(nUserUID, eMail_Room_BackChip, jsMailArg, eMailState_WaitSysAct);
	}
	break;
	case eAsync_player_League_Push_Event:
	{
		auto nUserUID = jsReqContent["targetUID"].asUInt();
		auto pPlayer = getPlayerByUserUID(nUserUID);
		if (pPlayer && pPlayer->isPlayerReady()) {
			Json::Value jsMsg;
			jsMsg["type"] = jsReqContent["type"];
			jsMsg["clubID"] = jsReqContent["clubID"];
			jsMsg["leagueID"] = jsReqContent["leagueID"];
			pPlayer->sendMsgToClient(jsMsg, MSG_LEAGUE_EVENT_ACTIVE_UPDATE);
		}
	}
	break;
	case eAsync_player_club_Push_Event:
	{
		auto nUserUID = jsReqContent["targetUID"].asUInt();
		auto pPlayer = getPlayerByUserUID(nUserUID);
		if (pPlayer && pPlayer->isPlayerReady()) {
			Json::Value jsMsg;
			jsMsg["type"] = jsReqContent["type"];
			jsMsg["clubID"] = jsReqContent["clubID"];
			pPlayer->sendMsgToClient(jsMsg, MSG_CLUB_EVENT_ACTIVE_UPDATE);
		}
	}
	break;
	case eAsync_player_game_add:
	{
		auto nUserUID = jsReqContent["targetUID"].asUInt();
		auto pPlayer = getPlayerByUserUID(nUserUID);
		if (pPlayer && pPlayer->isPlayerReady()) {
			bool isWin = jsReqContent["win"].asUInt();
			pPlayer->getBaseData()->addGameWin(isWin);
		}
		else {
			Json::Value jsMailArg;
			jsMailArg["uid"] = nUserUID;
			jsMailArg["win"] = jsReqContent["win"];
			auto pMailModule = ((DataServerApp*)getSvrApp())->getMailModule();
			pMailModule->postMail(nUserUID, eMail_Room_AddGame, jsMailArg, eMailState_WaitSysAct);
		}
	}
	break;
	case eAsync_player_do_DragIn:
	{
		auto nUserUID = jsReqContent["targetUID"].asUInt();
		uint32_t nAmount = jsReqContent["amount"].asUInt();
		bool bMTT = jsReqContent["mtt"].isUInt() ? jsReqContent["mtt"].asBool() : false;
		uint32_t nFee = 0;
		if (bMTT == false) {
			nFee = nAmount / 10;
			if (nFee % 10 > 0) {
				nFee += 1;
			}
		}
		nAmount += nFee;
		Json::Value jsMailArg;
		jsMailArg["uid"] = nUserUID;
		jsMailArg["amount"] = -1 * (int32_t)nAmount;
		jsMailArg["roomID"] = jsReqContent["roomID"];

		auto pMailModule = ((DataServerApp*)getSvrApp())->getMailModule();
		pMailModule->postMail(nUserUID, eMail_Room_DragInCoin, jsMailArg, eMailState_WaitSysAct);
	}
	break;
	case eAsync_Club_Quit:
	{
		auto nUserUID = jsReqContent["targetUID"].asUInt();
		Json::Value jsMailArg;
		jsMailArg["clubID"] = jsReqContent["clubID"];
		jsMailArg["uid"] = jsReqContent["uid"];

		auto pMailModule = ((DataServerApp*)getSvrApp())->getMailModule();
		pMailModule->postMail(nUserUID, eMail_Club_Quit, jsMailArg, eMailState_WaitSysAct);
	}
	break;
	case eAsync_Club_Fire:
	{
		auto nUserUID = jsReqContent["targetUID"].asUInt();
		Json::Value jsMailArg;
		jsMailArg["clubID"] = jsReqContent["clubID"];
		jsMailArg["uid"] = jsReqContent["uid"];

		auto pMailModule = ((DataServerApp*)getSvrApp())->getMailModule();
		pMailModule->postMail(nUserUID, eMail_Club_Fire, jsMailArg, eMailState_WaitSysAct);
	}
	break;
	case eAsync_Club_Dismiss:
	{
		auto nUserUID = jsReqContent["targetUID"].asUInt();
		Json::Value jsMailArg;
		jsMailArg["clubID"] = jsReqContent["clubID"];

		auto pMailModule = ((DataServerApp*)getSvrApp())->getMailModule();
		pMailModule->postMail(nUserUID, eMail_Club_Dismiss, jsMailArg, eMailState_WaitSysAct);
	}
	break;
	case eAsync_Club_Create:
	{
		uint32_t nUserUID = jsReqContent["targetUID"].asUInt();
		Json::Value jsMailArg;
		jsMailArg["clubID"] = jsReqContent["clubID"];

		auto pMailModule = ((DataServerApp*)getSvrApp())->getMailModule();
		pMailModule->postMail(nUserUID, eMail_Club_Create, jsMailArg, eMailState_WaitSysAct);
	}
	break;
	case eAsync_Club_Join:
	{
		uint32_t nUserUID = jsReqContent["targetUID"].asUInt();
		Json::Value jsMailArg;
		jsMailArg["clubID"] = jsReqContent["clubID"];
		jsMailArg["agentID"] = jsReqContent["agentID"];

		auto pMailModule = ((DataServerApp*)getSvrApp())->getMailModule();
		pMailModule->postMail(nUserUID, eMail_Club_Join, jsMailArg, eMailState_WaitSysAct);
	}
	break;
	case eAsync_Club_AddCoin:
	{
		uint32_t nUserUID = jsReqContent["targetUID"].asUInt();
		Json::Value jsMailArg;
		jsMailArg["clubID"] = jsReqContent["clubID"];
		jsMailArg["amount"] = jsReqContent["amount"];
		jsMailArg["agentID"] = jsReqContent["agentID"];

		auto pMailModule = ((DataServerApp*)getSvrApp())->getMailModule();
		pMailModule->postMail(nUserUID, eMail_Club_AddCoin, jsMailArg, eMailState_WaitSysAct);
	}
	break;
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
	case eAsync_AgentAddRoomCard:
	{
		uint32_t nUserUID = jsReqContent["targetUID"].asUInt();
		Json::Value jsMailArg;
		jsMailArg["agentID"] = jsReqContent["agentID"];
		jsMailArg["serialNo"] = jsReqContent["addCardNo"];
		jsMailArg["cardOffset"] = jsReqContent["addCard"];

		auto pMailModule = ((DataServerApp*)getSvrApp())->getMailModule();
		pMailModule->postMail(nUserUID, eMail_Agent_AddCard, jsMailArg, eMailState_WaitSysAct);
	}
	break;
	case eAsync_player_check_DragIn:
	{
		uint32_t nUserUID = jsReqContent["targetUID"].asUInt();
		auto pPlayer = getPlayerByUserUID(nUserUID);
		if (pPlayer == nullptr)  // player is not online , so should process in onAsyncRequestDelayResp
		{
			return false;
		}
		uint32_t nAmount = jsReqContent["amount"].asUInt();
		bool bMTT = jsReqContent["mtt"].isUInt() ? jsReqContent["mtt"].asBool() : false;
		uint32_t nFee = 0;
		if (bMTT == false) {
			nFee = nAmount / 10;
			if (nFee % 10 > 0) {
				nFee += 1;
			}
		}
		uint32_t nReal = nAmount + nFee;
		if (nReal > pPlayer->getBaseData()->getCoin()) {
			jsResult["ret"] = 1;
		}
		else {
			jsResult["ret"] = 0;
		}
	}
	break;
	case eAsync_AgentGetPlayerInfo:  // process , when target player is  online 
	{
		uint32_t nUserUID = jsReqContent["targetUID"].asUInt();
		auto pPlayer = getPlayerByUserUID(nUserUID);
		if ( pPlayer == nullptr )  // player is not online , so should process in onAsyncRequestDelayResp
		{
			return false;
		}

		jsResult["ret"] = 1;
		jsResult["isOnline"] = 1;
		jsResult["name"] = pPlayer->getBaseData()->getPlayerName();
		jsResult["leftCardCnt"] = pPlayer->getBaseData()->getDiamoned();
	}
	break;
	case eAsync_Consume_Diamond:
	{
		uint32_t nUserUID = jsReqContent["playerUID"].asUInt();
		Json::Value jsMailArg;
		jsMailArg["diamond"] = jsReqContent["diamond"];
		jsMailArg["roomID"] = jsReqContent["roomID"];
		jsMailArg["reason"] = jsReqContent["reason"];

		auto pMailModule = ((DataServerApp*)getSvrApp())->getMailModule();
		pMailModule->postMail(nUserUID, eMail_Consume_Diamond, jsMailArg, eMailState_WaitSysAct);
	}
	break;
	case eAsync_GiveBackDiamond:
	{
		uint32_t nUserUID = jsReqContent["targetUID"].asUInt();
		Json::Value jsMailArg;
		jsMailArg["diamond"] = jsReqContent["diamond"];
		jsMailArg["roomID"] = jsReqContent["roomID"];
		jsMailArg["reason"] = jsReqContent["reason"];

		auto pMailModule = ((DataServerApp*)getSvrApp())->getMailModule();
		pMailModule->postMail(nUserUID, eMail_GiveBack_Diamond, jsMailArg, eMailState_WaitSysAct);
	}
	break;
	case eAsync_Comsume_Golden_Emoji:
	{
		auto nUID = jsReqContent["targetUID"].asUInt();
		auto pPlayer = getPlayerByUserUID(nUID);
		if (!pPlayer || pPlayer->isPlayerReady() == false)
		{
			LOGFMTE(" can not find player uid = %u , to process async req = %u, let is time out", nUID, nRequestType);
			jsResult["ret"] = 2;
			break;
		}
		auto nComsumCnt = Emoji_Golden_Consume;
		if (nComsumCnt > pPlayer->getBaseData()->getCoin())
		{
			jsResult["ret"] = 1;
			break;
		}
		jsResult["ret"] = 0;

		uint32_t nUserUID = jsReqContent["targetUID"].asUInt();
		Json::Value jsMailArg;
		jsMailArg["roomID"] = jsReqContent["roomID"];
		jsMailArg["amount"] = -1 * (int32_t)Emoji_Golden_Consume;

		auto pMailModule = ((DataServerApp*)getSvrApp())->getMailModule();
		pMailModule->postMail(nUserUID, eMail_Consume_Golden_Emoji, jsMailArg, eMailState_WaitSysAct);
	}
	break;
	case eAsync_Comsume_Interact_Emoji:
	{
		auto nUID = jsReqContent["targetUID"].asUInt();
		auto pPlayer = getPlayerByUserUID(nUID);
		if (!pPlayer || pPlayer->isPlayerReady() == false )
		{
			LOGFMTE(" can not find player uid = %u , to process async req = %u, let is time out", nUID, nRequestType);
			jsResult["ret"] = 2;
			break;
		}

		auto nComsumCnt = jsReqContent["cnt"].asUInt();
		auto pBaseData = pPlayer->getBaseData();
		if ( nComsumCnt > pBaseData->getEmojiCnt())
		{
			jsResult["ret"] = 1;
			break;
		}

		jsResult["ret"] = 0;

		uint32_t nUserUID = jsReqContent["targetUID"].asUInt();
		Json::Value jsMailArg;
		jsMailArg["roomID"] = jsReqContent["roomID"];
		jsMailArg["cnt"] = jsReqContent["cnt"];

		auto pMailModule = ((DataServerApp*)getSvrApp())->getMailModule();
		pMailModule->postMail(nUserUID, eMail_Consume_Emoji, jsMailArg, eMailState_WaitSysAct);
	}
	break;
	case eAsync_Inform_Player_LeavedRoom:
	case eAsync_Request_EnterRoomInfo:
	case eAsync_Request_CreateRoomInfo:
	case eAsync_Inform_CreatedRoom:
	case eAsync_Inform_RoomDeleted:
	{
		auto nUID = jsReqContent["targetUID"].asUInt();
		auto pPlayer = getPlayerByUserUID(nUID);
		if (!pPlayer  )
		{
			LOGFMTE(" can not find player uid = %u , to process async req = %u, let is time out", nUID,nRequestType );
			jsResult["ret"] = 2;
			break;
		}
		return pPlayer->onAsyncRequest(nRequestType, jsReqContent, jsResult);
	}
	break;
	case eAsync_Check_WhiteList:
	{
		auto nUID = jsReqContent["listOwner"].asUInt();
		auto pPlayer = getPlayerByUserUID(nUID);
		if (!pPlayer)
		{
			LOGFMTE(" can not find player uid listowner = %u , to process async listOwner, let is time out", nUID );
			jsResult["ret"] = 1;
			break;
		}
		return pPlayer->onAsyncRequest(nRequestType, jsReqContent, jsResult);
	}
	break;
	case eAsync_HttpCmd_GetSvrInfo:
	{
		// player cnt ;
		jsResult["playerCnt"] = m_vAllActivePlayers.size();
		// active player cnt ;
		uint32_t nActCnt = 0;
		for (auto& ref : m_vAllActivePlayers)
		{
			if (ref.second && ref.second->isState(CPlayer::ePlayerState_Online))
			{
				++nActCnt;
			}
		}
		jsResult["activePlayerCnt"] = nActCnt;
		break;
	}
	break;
	case eAsync_HttpCmd_GetPlayerInfo:
	{
		if (jsReqContent["uid"].isNull() || jsReqContent["uid"].isUInt() == false)
		{
			jsResult["ret"] = 1;
			break;
		}

		auto nUserUID = jsReqContent["uid"].asUInt();
		auto p = getPlayerByUserUID(nUserUID);
		if (p == nullptr || p->isPlayerReady() == false )
		{
			jsResult["ret"] = 2;
			break;
		}

		jsResult["uid"] = nUserUID;
		jsResult["name"] = p->getBaseData()->getPlayerName();
		jsResult["icon"] = p->getBaseData()->getHeadIcon();
		jsResult["diamond"] = p->getBaseData()->getDiamoned();
		jsResult["emojiCnt"] = p->getBaseData()->getEmojiCnt();
		auto gd = (CPlayerGameData*)p->getComponent(ePlayerComponent_PlayerGameData);
		gd->adminVisitInfo(jsResult);
		jsResult["ret"] = 0;
	}
	break;
	case eAsync_HttpCmd_AddEmojiCnt:
	{
		if (jsReqContent["targetUID"].isUInt() == false || false == jsReqContent["agentID"].isUInt() || false == jsReqContent["addCnt"].isUInt())
		{
			jsResult["ret"] = 1;
			break;
		}

		uint32_t nUserUID = jsReqContent["targetUID"].asUInt();

		auto p = getPlayerByUserUID(nUserUID);
		if (p == nullptr || p->isPlayerReady() == false)
		{
			jsResult["ret"] = 2;
			break;
		}

		Json::Value jsMailArg;
		jsMailArg["agentID"] = jsReqContent["agentID"];
		jsMailArg["addCnt"] = jsReqContent["addCnt"];

		auto pMailModule = ((DataServerApp*)getSvrApp())->getMailModule();
		pMailModule->postMail(nUserUID, eMail_Agent_AddEmojiCnt, jsMailArg, eMailState_WaitSysAct);
	}
	break;
	default:
	{
		if (jsReqContent["playerUID"].isNull() == false)
		{
			auto nPlayerUID = jsReqContent["playerUID"].asUInt();
			auto pPlayer = getPlayerByUserUID(nPlayerUID);
			if (pPlayer == nullptr)
			{
				jsResult["ret"] = 1; // can not find target player ;
				return false;
			}
			else
			{
				return pPlayer->onAsyncRequest(nRequestType, jsReqContent, jsResult);
			}
		}
		else {
			return false;
		}
	}

	}
	return true ;
}

bool CPlayerManager::onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID, uint16_t nTargetID )
{
	if (eAsync_HttpCmd_UIDRoomInfo == nRequestType) {
		auto pApp = getSvrApp();
		uint32_t nUserUID = jsReqContent["uid"].isUInt() ? jsReqContent["uid"].asUInt() : 0;
		if (nUserUID == 0) {
			Json::Value jsAgentBack;
			jsAgentBack["ret"] = 1;
			pApp->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsAgentBack, nUserUID);
			return true;
		}
		auto pPlayer = getPlayerByUserUID(nUserUID);
		if (pPlayer == nullptr) {
			Json::Value jsAgentBack;
			jsAgentBack["ret"] = 1;
			pApp->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsAgentBack, nUserUID);
			return true;
		}
		auto pStay = ((CPlayerGameData*)pPlayer->getComponent(ePlayerComponent_PlayerGameData))->getStayInRoom();
		if (pStay.isEmpty())
		{
			Json::Value jsAgentBack;
			jsAgentBack["ret"] = 2;
			pApp->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsAgentBack, nUserUID);
			return true;
		}
		Json::Value jsReq;
		jsReq["roomID"] = pStay.nRoomID;
		pApp->getAsynReqQueue()->pushAsyncRequest(pStay.nSvrPort, pStay.nRoomID, eAsync_HttpCmd_RoomIDRoomInfo, jsReq, [nReqSerial, nSenderID, nSenderPort, pApp, nUserUID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut) {
			Json::Value jsAgentBack;
			if (isTimeOut)
			{
				jsAgentBack["ret"] = 7;
				pApp->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsAgentBack, nUserUID);
				LOGFMTE(" request time out , so uidroominfo can not find , uid = %u", nUserUID);
			}

			jsAgentBack = retContent;
			pApp->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsAgentBack, nUserUID);
		});
		return true;
	}

	if (eAsync_player_check_DragIn == nRequestType) {
		uint32_t nUserUID = jsReqContent["targetUID"].asUInt();
		auto pPlayer = getPlayerByUserUID(nUserUID);
		if (nullptr != pPlayer)  // shold process in onAsyncRequest
		{
			return false;
		}
		uint32_t nAmount = jsReqContent["amount"].asUInt();
		bool bMTT = jsReqContent["mtt"].isUInt() ? jsReqContent["mtt"].asBool() : false;
		uint32_t nFee = 0;
		if (bMTT == false) {
			nFee = nAmount / 10;
			if (nFee % 10 > 0) {
				nFee += 1;
			}
		}
		nAmount += nFee;
		auto pApp = getSvrApp();
		LOGFMTD("uid = %u not online get check drag in from db ", nUserUID);
		Json::Value jsSql;
		char pBuffer[512] = { 0 };
		sprintf_s(pBuffer, sizeof(pBuffer), "SELECT coin FROM playerbasedata WHERE userUID = %u ;", nUserUID);
		jsSql["sql"] = pBuffer;
		pApp->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB, nSenderID, eAsync_DB_Select, jsSql, [nReqSerial, nSenderID, nSenderPort, pApp, nUserUID, nAmount](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
		{
			uint8_t nRow = retContent["afctRow"].asUInt();
			if (isTimeOut)
			{
				nRow = 0;
			}

			if (nRow == 0)
			{
				Json::Value jsAgentBack;
				jsAgentBack["ret"] = 7;
				LOGFMTE(" request time out or can not find info , so can not check drag in, uid = %u", nUserUID);
				pApp->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsAgentBack, nUserUID);
				return;
			}

			Json::Value jsData = retContent["data"];
			Json::Value jsRow = jsData[0u];
			int32_t nCoin = jsRow["coin"].asInt();
			//LOGFMTD("uid = %u base data card cnt = %u", nUserUID, jsRow["diamond"].asUInt());

			// take not process add coin mail in to account 
			Json::Value jsSql;
			char pBuffer[512] = { 0 };
			sprintf_s(pBuffer, sizeof(pBuffer), "SELECT mailDetail FROM mail WHERE userUID = %u and (mailType = %u or mailType = %u or mailType = %u or mailType = %u or mailType = %u or mailType = %u) and state = %u limit 10 ;", nUserUID, eMail_Club_AddCoin, eMail_Room_DragInCoin, eMail_Owner_Pay, eMail_Room_BackChip, eMail_Room_RotBanker, eMail_Room_ShowCards, eMailState_WaitSysAct);
			jsSql["sql"] = pBuffer;
			pApp->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DB, nUserUID, eAsync_DB_Select, jsSql, [nSenderPort, nReqSerial, nSenderID, nUserUID, pApp, nCoin, nAmount](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
			{
				if (isTimeOut)
				{
					Json::Value jsAgentBack;
					jsAgentBack["ret"] = 7;
					LOGFMTE(" request time out or can not find info , so can not check drag in, uid = %u", nUserUID);
					pApp->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsAgentBack, nUserUID);
					return;
				}

				int32_t nTotalCnt = 0;
				uint8_t nRow = retContent["afctRow"].asUInt();
				Json::Value jsData = retContent["data"];
				for (uint8_t nIdx = 0; nIdx < jsData.size(); ++nIdx)
				{
					Json::Value jsRow = jsData[nIdx];

					Json::Reader jsReader;
					Json::Value jsC;
					auto bRt = jsReader.parse(jsRow["mailDetail"].asString(), jsC);
					if (!bRt || jsC["amount"].isNull())
					{
						LOGFMTE("pasre add coin mail error id = %u", nUserUID);
						continue;
					}
					nTotalCnt += jsC["amount"].asInt();
				}
				LOGFMTD("uid = %u mail coin = %u", nUserUID, nTotalCnt);
				nTotalCnt += nCoin;
				Json::Value jsAgentBack;
				if (nTotalCnt < (int32_t)nAmount) {
					jsAgentBack["ret"] = 1;
				}
				else {
					jsAgentBack["ret"] = 0;
				}
				pApp->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsAgentBack, nUserUID);
			});
		});
		return true;
	}

	if ( eAsync_AgentGetPlayerInfo == nRequestType)
	{
		uint32_t nUserUID = jsReqContent["targetUID"].asUInt();
		auto pPlayer = getPlayerByUserUID(nUserUID);
		if ( nullptr != pPlayer)  // shold process in onAsyncRequest
		{
			return false;
		}
		auto async = getSvrApp()->getAsynReqQueue();
		LOGFMTD("uid = %u not online get info from db ", nUserUID );
		// not online , must get name first ;
		Json::Value jsSql;
		char pBuffer[512] = { 0 };
		sprintf_s(pBuffer,sizeof(pBuffer), "SELECT nickName, diamond FROM playerbasedata WHERE userUID = %u ;", nUserUID );
		jsSql["sql"] = pBuffer;

		async->pushAsyncRequest(ID_MSG_PORT_DB,nSenderID,eAsync_DB_Select, jsSql, [nReqSerial, nSenderID,async, nUserUID,this ](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut )
		{
			uint8_t nRow = retContent["afctRow"].asUInt();
			if (isTimeOut)
			{
				nRow = 0; 
				LOGFMTE(" request time out , so regart as can not find , uid = %u",nUserUID );
			}

			Json::Value jsAgentBack;
			jsAgentBack["ret"] = 1;
			jsAgentBack["isOnline"] = 0;
			jsAgentBack["name"] = "";
			jsAgentBack["leftCardCnt"] = 0;

			if (nRow == 0)
			{
				jsAgentBack["ret"] = 0;
				getSvrApp()->responeAsyncRequest(ID_MSG_PORT_VERIFY, nReqSerial,nSenderID, jsAgentBack, nUserUID );
				LOGFMTE("get can find uid = %u info from db", nUserUID);
				return;
			}

			Json::Value jsData = retContent["data"];
			Json::Value jsRow = jsData[0u];
			jsAgentBack["name"] = jsRow["nickName"];
			jsAgentBack["leftCardCnt"] = jsRow["diamond"];
			LOGFMTD("uid = %u base data card cnt = %u", nUserUID, jsRow["diamond"].asUInt());

			// take not process add card mail in to account 
			Json::Value jsSql;
			char pBuffer[512] = { 0 };
			sprintf_s(pBuffer, sizeof(pBuffer),"SELECT mailDetail FROM mail WHERE userUID = %u and mailType = %u and state = %u limit 10 ;", nUserUID, eMail_Agent_AddCard, eMailState_WaitSysAct);
			jsSql["sql"] = pBuffer;
			auto pSvrApp = async->getSvrApp();
			async->pushAsyncRequest(ID_MSG_PORT_DB, nUserUID,eAsync_DB_Select, jsSql, [nReqSerial, nSenderID, nUserUID, pSvrApp](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut )
			{
				if (isTimeOut)
				{
					return;
				}

				int32_t nTotalCnt = 0;
				uint8_t nRow = retContent["afctRow"].asUInt();
				Json::Value jsData = retContent["data"];
				for (uint8_t nIdx = 0; nIdx < jsData.size(); ++nIdx)
				{
					Json::Value jsRow = jsData[nIdx];

					Json::Reader jsReader;
					Json::Value jsC;
					auto bRt = jsReader.parse(jsRow["mailDetail"].asString(), jsC);
					if (!bRt || jsC["cardOffset"].isNull())
					{
						LOGFMTE("pasre add card mail error id = %u", nUserUID);
						continue;
					}
					nTotalCnt += jsC["cardOffset"].asInt();
				}
				LOGFMTD("uid = %u mail card cnt = %u", nUserUID, nTotalCnt);
				jsUserData["leftCardCnt"] = jsUserData["leftCardCnt"].asInt() + nTotalCnt;

				// build msg to send ;
				pSvrApp->responeAsyncRequest(ID_MSG_PORT_VERIFY, nReqSerial, nSenderID, jsUserData, nUserUID );
				LOGFMTD("do get player info cards uid = %u", nUserUID);
			}, jsAgentBack);
		});
		return true;
	}

	if (jsReqContent["playerUID"].isNull() == false)
	{
		auto nPlayerUID = jsReqContent["playerUID"].asUInt();
		auto pPlayer = getPlayerByUserUID(nPlayerUID);
		if (pPlayer == nullptr)
		{
			return false;
		}
		else
		{
			return pPlayer->onAsyncRequestDelayResp(nRequestType, nReqSerial, jsReqContent, nSenderPort, nSenderID);
		}
	}

	return false;
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
	 m_vWillDeletePlayers.clear();
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
		return false;
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
			if ( ref.second->getSessionID() % nSvrMaxCnt == nSvrIdx )
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

	for ( auto& ref : m_vAllActivePlayers )
	{
		if (ref.second)
		{
			ref.second->onOtherSvrShutDown(nSvrPort, nSvrIdx, nSvrMaxCnt);
		}
	}
	return false;
}