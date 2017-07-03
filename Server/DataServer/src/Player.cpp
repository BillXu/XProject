#include "Player.h"
#include "log4z.h"
#include "PlayerManager.h"
#include "GameServerApp.h"
#include "PlayerBaseData.h"
#include "PlayerMail.h"
#include "PlayerItem.h"
#include "PlayerMission.h"
#include "Timer.h"
#include "PlayerShop.h"
#include "ServerMessageDefine.h"
#include "RobotManager.h"
#include "PlayerFriend.h"
#include "ServerCommon.h"
#include "AutoBuffer.h"
#include "RoomConfig.h"
#include "PlayerGameData.h"
#define TIME_SAVE 60*10
#define TIME_DELAY_DELETE 2*60
CPlayer::CPlayer( )
{
	m_nUserUID = 0 ;
	m_eSate = ePlayerState_Online ;
	m_nSessionID = 0 ;
	for ( int i = ePlayerComponent_None; i < ePlayerComponent_Max ; ++i )
	{
		 m_vAllComponents[i] = NULL;
	}
}

CPlayer::~CPlayer()
{
	for ( int i = ePlayerComponent_None; i < ePlayerComponent_Max ; ++i )
	{
		IPlayerComponent* p = m_vAllComponents[i] ;
		if ( p )
			delete p ;
		m_vAllComponents[i] = NULL ;
	}
}

void CPlayer::init(unsigned int nUserUID, unsigned int nSessionID )
{
	m_nSessionID = nSessionID ;
	m_nUserUID = nUserUID ;
	m_eSate = ePlayerState_Online ;
	/// new components ;here ;
	m_vAllComponents[ePlayerComponent_BaseData] = new CPlayerBaseData(this) ;
	m_vAllComponents[ePlayerComponent_Mail] = new CPlayerMailComponent(this);
	m_vAllComponents[ePlayerComponent_PlayerGameData] = new CPlayerGameData(this);
	//m_vAllComponents[ePlayerComponent_PlayerItemMgr] = new CPlayerItemComponent(this);
	//m_vAllComponents[ePlayerComponent_PlayerMission] = new CPlayerMission(this);
	//m_vAllComponents[ePlayerComponent_PlayerShop] = new CPlayerShop(this);
	for ( int i = ePlayerComponent_None; i < ePlayerComponent_Max ; ++i )
	{
		IPlayerComponent* p = m_vAllComponents[i] ;
		if ( p )
		{
			p->init();
		}
	}

	m_tTimerSave.setCallBack(timer_bind_obj_func(this,CPlayer::onTimerSave )) ;
	m_tTimerSave.setIsAutoRepeat(true) ;
	m_tTimerSave.setInterval(TIME_SAVE);
	m_tTimerSave.start() ;
}

void CPlayer::reset(unsigned int nUserUID, unsigned int nSessionID )
{
	m_nSessionID = nSessionID ;
	m_nUserUID = nUserUID ;
	m_eSate = ePlayerState_Online ;
	// inform components;
	for ( int i = ePlayerComponent_None; i < ePlayerComponent_Max ; ++i )
	{
		IPlayerComponent* p = m_vAllComponents[i] ;
		if ( p )
		{
			p->reset();
		}
	}

	m_tTimerSave.reset();
	m_tTimerSave.start() ;
}

bool CPlayer::onMsg( stMsg* pMsg , eMsgPort eSenderPort, uint32_t nSenderID )
{
	if ( processPublicPlayerMsg(pMsg,eSenderPort) )
	{
		return true; 
	}

	for ( int i = ePlayerComponent_None; i < ePlayerComponent_Max ; ++i )
	{
		IPlayerComponent* p = m_vAllComponents[i] ;
		if ( p )
		{
			if ( p->onMsg(pMsg,eSenderPort) )
			{
				return true;
			}
		}
	}

	LOGFMTE("Unprocessed msg id = %d, from = %d  uid = %d",pMsg->usMsgType,eSenderPort,getUserUID() ) ;

	return false ;

}

bool CPlayer::onMsg( Json::Value& recvValue , uint16_t nmsgType, eMsgPort eSenderPort, uint32_t nSenderID )
{
	for ( int i = ePlayerComponent_None; i < ePlayerComponent_Max ; ++i )
	{
		IPlayerComponent* p = m_vAllComponents[i] ;
		if ( p )
		{
			if ( p->onMsg(recvValue,nmsgType,eSenderPort) )
			{
				return true;
			}
		}
	}

	LOGFMTE("Unprocessed msg id = %d, from = %d  uid = %d",nmsgType,eSenderPort,getUserUID() ) ;

	return false ;
}

void CPlayer::onPlayerDisconnect()
{
	// inform components;
	for ( int i = ePlayerComponent_None; i < ePlayerComponent_Max ; ++i )
	{
		IPlayerComponent* p = m_vAllComponents[i] ;
		if ( p )
		{
			p->onPlayerDisconnect();
		}
	}

	onTimerSave(nullptr,0);
	m_tTimerSave.canncel() ;

	setState(ePlayerState_Offline) ;
	LOGFMTE("player disconnect should inform other sever");

	// save log 
	//stMsgSaveLog msgLog ;
	//memset(msgLog.vArg,0,sizeof(msgLog.vArg));
	//msgLog.nJsonExtnerLen = 0 ;
	//msgLog.nLogType = eLog_PlayerLogOut ;
	//msgLog.nTargetID = GetUserUID() ;
	//memset(msgLog.vArg,0,sizeof(msgLog.vArg));
	//msgLog.vArg[0] = GetBaseData()->getCoin() ;
	//msgLog.vArg[1] = GetBaseData()->GetAllDiamoned();
	//SendMsgToClient((char*)&msgLog,sizeof(msgLog));
}

void CPlayer::postPlayerEvent(stPlayerEvetArg* pEventArg )
{
	for ( int i = ePlayerComponent_None; i < ePlayerComponent_Max ; ++i )
	{
		IPlayerComponent* p = m_vAllComponents[i] ;
		if ( p )
		{
			p->onPlayerEvent(pEventArg);
		}
	}
}

void CPlayer::sendMsgToClient(const char* pBuffer, unsigned short nLen  )
{
	stMsg* pmsg = (stMsg*)pBuffer ;
	if ( isState(ePlayerState_Offline) == false )
	{
		CGameServerApp::SharedGameServerApp()->sendMsg(getSessionID(),pBuffer,nLen ) ;
		return ;
	}
	LOGFMTD("player uid = %d not online so , can not send msg" ,getUserUID() ) ;
}

void CPlayer::sendMsgToClient(Json::Value& jsMsg, uint16_t nMsgType  )
{
	if ( isState(ePlayerState_Offline) == false  )
	{
		CGameServerApp::SharedGameServerApp()->sendMsg(GetSessionID(),jsMsg,nMsgType,ID_MSG_PORT_CLIENT,bBrocat) ;
		return ;
	}
	LOGFMTD("player uid = %d not online so , can not send msg" ,GetUserUID() ) ;
}

bool CPlayer::isState( ePlayerState eState )
{
	if ( (m_eSate & eState) == eState )
	{
		return true ;
	}
	return false ;
}

void CPlayer::onAnotherClientLoginThisPeer(unsigned int nSessionID )
{
	// tell prelogin client to disconnect ;
	stMsgPlayerOtherLogin msg ;
	SendMsgToClient((char*)&msg,sizeof(msg)) ;

	LOGFMTE("pls remember inform other server this envent OnAnotherClientLoginThisPeer ") ;

	for ( int i = ePlayerComponent_None; i < ePlayerComponent_Max ; ++i )
	{
		IPlayerComponent* p = m_vAllComponents[i] ;
		if ( p )
		{
			p->onOtherWillLogined();
		}
	}
	// bind new client ;
	m_nSessionID = nSessionID ;
	m_eSate = ePlayerState_Online;
	for ( int i = ePlayerComponent_None; i < ePlayerComponent_Max ; ++i )
	{
		IPlayerComponent* p = m_vAllComponents[i] ;
		if ( p )
		{
			p->onOtherDoLogined();
		}
	}
}

bool CPlayer::processPublicPlayerMsg(stMsg* pMsg , eMsgPort eSenderPort)
{
	switch ( pMsg->usMsgType )
	{
	case MSG_ADD_MONEY:
		{

		}
		break;
// 	case MSG_REQUEST_RANK:
// 		{
// 			stMsgPlayerRequestRank* pMsgRet = (stMsgPlayerRequestRank*)pMsg ;
// 			CGameServerApp::SharedGameServerApp()->GetGameRanker()->SendRankToPlayer(this,(eRankType)pMsgRet->nRankType,pMsgRet->nFromIdx,pMsgRet->nCount ) ;
// 		}
// 		break;
// 	case MSG_REQUEST_RANK_PEER_DETAIL:
// 		{
// 			stMsgPlayerRequestRankPeerDetail* pRetMsg = (stMsgPlayerRequestRankPeerDetail*)pMsg ;
// 			CGameServerApp::SharedGameServerApp()->GetGameRanker()->SendRankDetailToPlayer(this,pRetMsg->nRankPeerUID,(eRankType)pRetMsg->nRankType);
// 		}
// 		break;
// 	case MSG_PLAYER_SAY_BROCAST:
// 		{
// 			stMsgPlayerSayBrocast* pMsgRet = (stMsgPlayerSayBrocast*)pMsg ;
// 			CPlayerItemComponent* pItemMgr = (CPlayerItemComponent*)GetComponent(ePlayerComponent_PlayerItemMgr);	
// 			stMsgPlayerSayBrocastRet msg ;
// 			msg.nRet = 0 ;
// 			if ( pItemMgr->OnUserItem(ITEM_ID_LA_BA) )
// 			{
// 				CGameServerApp::SharedGameServerApp()->GetBrocaster()->PostPlayerSayMsg(this,((char*)pMsg) + sizeof(stMsgPlayerSayBrocast),pMsgRet->nContentLen) ;
// 			}
// 			else
// 			{
// 				msg.nRet = 1 ;
// 				LOGFMTE(" you have no la ba") ;
// 			}
// 			SendMsgToClient((char*)&msg,sizeof(msg)) ;
// 		}
// 		break;
// 	case MSG_PLAYER_REPLAY_BE_INVITED:
// 		{
// 			//stMsgPlayerRecievedInviteReply toMsgInviter; // who invite me ;
// 			stMsgPlayerReplayBeInvitedToJoinRoom* pMsgRet = (stMsgPlayerReplayBeInvitedToJoinRoom*)pMsg ;
// 			//toMsgInviter.nRet = 0 ;
// 			stMsgPlayerReplayBeInvitedToJoinRoomRet msgBack ;
// 			if ( pMsgRet->nReplyResult == 1 ) // refused 
// 			{
// 				//toMsgInviter.nRet = 1 ;
// 				msgBack.nRet = 0 ;
// 				SendMsgToClient((char*)&msgBack,sizeof(msgBack)) ;
// 				break;
// 			}
// 			else  // i agreed ;
// 			{
// 				msgBack.nRet = 0 ;
// 				CTaxasPokerPeer* pThisPeer = (CTaxasPokerPeer*)GetComponent(ePlayerComponent_RoomPeerTaxasPoker);
// 				CRoomTexasPoker* pRoomToEnter = (CRoomTexasPoker*)CGameServerApp::SharedGameServerApp()->GetRoomMgr()->GetRoom(pMsgRet->nRoomType,pMsgRet->nRoomLevel,pMsgRet->nRoomID) ;
// 				if ( !pRoomToEnter || pRoomToEnter->CanPeerSitDown(pThisPeer) == false )
// 				{
// 					//toMsgInviter.nRet = 4 ;
// 					msgBack.nRet = 2 ;
// 				}
// 				else
// 				{
// 					// join room ;
// 					pThisPeer->LeaveRoom();
// 					if ( pRoomToEnter->AddBeInvitedPlayer(this,pMsgRet->nSitIdx) == false )
// 					{
// 						//toMsgInviter.nRet = 3 ;
// 						msgBack.nRet = 3 ;
// 					}
// 				}
// 
// 				if ( msgBack.nRet != 0 )  // only failed , tell client , when success , directly enter room ;
// 				{
// 					SendMsgToClient((char*)&msgBack,sizeof(msgBack)) ;
// 				}
// 			}
// 			//CPlayer* pToInvid = CGameServerApp::SharedGameServerApp()->GetPlayerMgr()->GetPlayerByUserUID(pMsgRet->nReplyToUserUID) ;
// 			//if ( pToInvid && toMsgInviter.nRet != 0 ) // only failed situation ,tell inviter ;
// 			//{
// 			//	memcpy(toMsgInviter.nReplyerName,GetBaseData()->GetPlayerName(),MAX_LEN_CHARACTER_NAME);
// 			//	pToInvid->SendMsgToClient((char*)&toMsgInviter,sizeof(toMsgInviter)) ;
// 			//}
// 			//else
// 			//{
// 			//	LOGFMTD("the one who invite me had offline , his uid = %d",pMsgRet->nReplyToUserUID) ;
// 			//}
// 		}
// 		break;
// 	case MSG_PLAYER_FOLLOW_TO_ROOM:
// 		{
// 			stMsgPlayerFollowToRoom* pRetMsg = (stMsgPlayerFollowToRoom*)pMsg ;
// 			CPlayer* pTargetPlayer = CGameServerApp::SharedGameServerApp()->GetPlayerMgr()->GetPlayerByUserUID(pRetMsg->nTargetPlayerUID) ;
// 			stMsgPlayerFollowToRoomRet msgBack ;
// 			msgBack.nRet = 0 ;
// 			if ( pTargetPlayer == NULL )
// 			{
// 				msgBack.nRet = 1 ;
// 				SendMsgToClient((char*)&msgBack,sizeof(msgBack)) ;
// 				break ;
// 			}
// 
// 			if ( ePlayerState_Free == pTargetPlayer->GetState() )
// 			{
// 				msgBack.nRet = 2 ;
// 				SendMsgToClient((char*)&msgBack,sizeof(msgBack)) ;
// 				break ;
// 			}
// 
// 			if ( ePlayerState_Free != GetState() )
// 			{
// 				msgBack.nRet = 4 ;
// 				SendMsgToClient((char*)&msgBack,sizeof(msgBack)) ;
// 				break ;
// 			}
// 
// 			CRoomBaseNew* pStateRoom = pTargetPlayer->GetRoomCurStateIn() ;
// 			if ( !pStateRoom )
// 			{
// 				LOGFMTE("follow to a null room , but target player is not free , how , why ?") ;
// 				msgBack.nRet = 2 ;
// 				SendMsgToClient((char*)&msgBack,sizeof(msgBack)) ;
// 				break;
// 			}
// 
// 
// 			if ( pStateRoom->CheckCanJoinThisRoom(pTargetPlayer) != 0)
// 			{
// 				msgBack.nRet = 3 ;
// 				SendMsgToClient((char*)&msgBack,sizeof(msgBack)) ;
// 				break;
// 			}
// 
// 			// add to room 
// 			stMsgRoomEnter msgToEnterRoom ;
// 			msgToEnterRoom.nPassword = 0 ;
// 			msgToEnterRoom.nRoomID = pStateRoom->GetRoomID();
// 			msgToEnterRoom.nRoomLevel = pStateRoom->GetRoomLevel() ;
// 			msgToEnterRoom.nRoomType = pStateRoom->GetRoomType() ;
// 			OnMessage(&msgToEnterRoom);
// 		}
// 		break;
// 	case MSG_PLAYER_SLOT_MACHINE:
// 		{
// 			stMsgPlayerSlotMachineRet msgBack ;
// 			msgBack.nRet = 0 ;
// 			stMsgPlayerSlotMachine* pRetMsg = (stMsgPlayerSlotMachine*)pMsg ;
// 			if ( GetBaseData()->GetAllCoin() < pRetMsg->nBetCoin || GetBaseData()->GetAllDiamoned() < pRetMsg->nBetDiamoned )
// 			{
// 				msgBack.nRet = 1 ;
// 			}
// 			else
// 			{
// 				CSlotMachine* pMachine = (CSlotMachine*)CGameServerApp::SharedGameServerApp()->GetConfigMgr()->GetConfig(CConfigManager::eConfig_SlotMachine) ;
// 				float fRate = 0 ;
// 				pMachine->RandSlotMachine(pRetMsg->cLevel,msgBack.vCard,fRate) ;
// 				int64_t nOffsetCoin = pRetMsg->nBetCoin * ( fRate - 1.0 );
// 				int nOffsetDiamoned = pRetMsg->nBetDiamoned * ( fRate - 1.0 );
// 				nOffsetDiamoned = abs(nOffsetDiamoned) ;
// 				nOffsetCoin = abs(nOffsetCoin) ;
// 				int nOffset = fRate > 1 ? 1 : -1 ;
// 				GetBaseData()->ModifyMoney(nOffsetCoin * nOffset);
// 				GetBaseData()->ModifyMoney(nOffsetDiamoned * nOffset,true);				
// 				msgBack.nFinalAllCoin = GetBaseData()->GetAllCoin();
// 				msgBack.nFinalDiamoned = GetBaseData()->GetAllDiamoned() ;
// 				msgBack.nTakeInCoin = GetBaseData()->GetTakeInMoney() ;
// 				msgBack.nTakeInDiamoned = GetBaseData()->GetTakeInMoney(true) ;
// 				SendMsgToClient((char*)&msgBack,sizeof(msgBack)) ;
// 			}
// 		}
// 		break;
// 	case MSG_ROOM_REQUEST_PEER_DETAIL:
// 		{
// 			stMsgRoomRequestPeerDetailRet msgBack ;
// 			stMsgRoomRequestPeerDetail* pMsgRet = (stMsgRoomRequestPeerDetail*)pMsg ;
// 			msgBack.nPeerSessionID = pMsgRet->nPeerSessionID ;
// 			CPlayer* pDetailPlayer = CGameServerApp::SharedGameServerApp()->GetPlayerMgr()->GetPlayerBySessionID(pMsgRet->nPeerSessionID) ;
// 			if ( pDetailPlayer == NULL )
// 			{
// 				msgBack.nRet = 1 ;
// 				SendMsgToClient((char*)&msgBack,sizeof(msgBack)) ;
// 				return true ;
// 			}
// 
// 			pDetailPlayer->GetBaseData()->GetPlayerDetailData(&msgBack.stDetailInfo);
// 			SendMsgToClient((char*)&msgBack,sizeof(msgBack)) ;
// 		}
// 		break;
 	default:
 		return false ;
	}
	return true ;
}

void CPlayer::onTimerSave( CTimer* p,float fTimeElaps )
{
	for ( int i = ePlayerComponent_None; i < ePlayerComponent_Max ; ++i )
	{
		IPlayerComponent* p = m_vAllComponents[i] ;
		if ( p )
		{
			p->timerSave();
		}
	}
}

//void CPlayer::PushTestAPNs()
//{
//#ifdef NDEBUG
//	return ;
//#endif
//	//if ( GetBaseData()->bPlayerEnableAPNs == false )
//	//{
//	//	LOGFMTE("you not enable apns ") ;
//	//	return ;
//	//}
//	//char* pString = "\"you disconnected \"" ;
//	//stMsgToAPNSServer msg ;
//	//msg.nAlertLen = strlen(pString) ;
//	//msg.nBadge = 1 ;
//	//msg.nSoundLen = 0 ;
//	//memcpy(msg.pDeveiceToken,GetBaseData()->vAPNSToken,32);
//	//char* pBuffer = new char[sizeof(msg) + msg.nAlertLen ] ;
//	//unsigned short nOffset = 0 ;
//	//memcpy(pBuffer,&msg,sizeof(msg));
//	//nOffset += sizeof(msg);
//	//memcpy(pBuffer + nOffset , pString ,msg.nAlertLen);
//	//nOffset += msg.nAlertLen ;
//	//CGameServerApp::SharedGameServerApp()->SendMsgToAPNsServer(pBuffer,nOffset);
//	//delete[] pBuffer ;
//}

void CPlayer::onReactive(uint32_t nSessionID )
{
	LOGFMTD("uid = %d reactive with session id = %d", getUserUID(), nSessionID) ;
	m_nSessionID = nSessionID ;
	setState(ePlayerState_Online) ;

	for ( int i = ePlayerComponent_None; i < ePlayerComponent_Max ; ++i )
	{
		IPlayerComponent* p = m_vAllComponents[i] ;
		if ( p )
		{
			p->onReactive(nSessionID);
		}
	}
	m_tTimerSave.reset() ;
	m_tTimerSave.start() ;
}

uint8_t CPlayer::getMsgPortByRoomType(uint8_t nType )
{
	switch ( nType )
	{
	case eRoom_NiuNiu:
		return ID_MSG_PORT_NIU_NIU ;
	case eRoom_TexasPoker:
		return ID_MSG_PORT_TAXAS ;
	case eRoom_Golden:
		return ID_MSG_PORT_GOLDEN;
	case eRoom_MJ_Blood_River:
	case eRoom_MJ_Blood_End:
	case eRoom_MJ_NanJing:
	case eRoom_MJ_SuZhou:
		return ID_MSG_PORT_MJ;
	default:
		return ID_MSG_PORT_NONE ;
	}

	return ID_MSG_PORT_NONE ;
}



