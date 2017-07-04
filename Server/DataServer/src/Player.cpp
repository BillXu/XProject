#include "Player.h"
#include "log4z.h"
#include "PlayerManager.h"
#include "PlayerBaseData.h"
#include "PlayerMail.h"
#include "ServerMessageDefine.h"
#include "ServerCommon.h"
#include "AutoBuffer.h"
#include "RoomConfig.h"
#include "PlayerGameData.h"
#include "AsyncRequestQuene.h"
#include "ISeverApp.h"
#define TIME_DELAY_DELETE 2*60
CPlayer::CPlayer()
{
	m_nUserUID = 0 ;
	m_eSate = ePlayerState_Online ;
	m_nSessionID = 0 ;
	m_strCurIP = "";
	m_pPlayerMgr = nullptr;
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

void CPlayer::init( CPlayerManager* pPlayerMgr)
{
	m_pPlayerMgr = pPlayerMgr;
	m_eSate = ePlayerState_Online ;
	m_strCurIP = "";
	/// new components ;here ;
	m_vAllComponents[ePlayerComponent_BaseData] = new CPlayerBaseData(this) ;
	m_vAllComponents[ePlayerComponent_Mail] = new CPlayerMailComponent(this);
	m_vAllComponents[ePlayerComponent_PlayerGameData] = new CPlayerGameData(this);
	for ( int i = ePlayerComponent_None; i < ePlayerComponent_Max ; ++i )
	{
		IPlayerComponent* p = m_vAllComponents[i] ;
		if ( p )
		{
			p->init();
		}
	}
}

void CPlayer::reset()
{
	m_nSessionID = 0 ;
	m_nUserUID = 0 ;
	m_strCurIP = "";
	m_eSate = ePlayerState_Online ;
	m_tTimerCheckRemovePlayer.canncel();
	m_tTimerCheckRemovePlayer.reset();
	// inform components;
	for ( int i = ePlayerComponent_None; i < ePlayerComponent_Max ; ++i )
	{
		IPlayerComponent* p = m_vAllComponents[i] ;
		if ( p )
		{
			p->reset();
		}
	}
}

void CPlayer::onPlayerLogined(uint32_t nSessionID, uint32_t nUserUID, const char* pIP)
{
	m_nSessionID = nSessionID;
	m_nUserUID = nUserUID;
	m_strCurIP = pIP;
	setState(ePlayerState_Online);
	for (int i = ePlayerComponent_None; i < ePlayerComponent_Max; ++i)
	{
		IPlayerComponent* p = m_vAllComponents[i];
		if (p)
		{
			p->onPlayerLogined();
		}
	}
	saveLoginInfo();
}

void CPlayer::onPlayerReconnected(char* pNewIP)
{
	m_strCurIP = pNewIP;
	setState(ePlayerState_Online);
	for (int i = ePlayerComponent_None; i < ePlayerComponent_Max; ++i)
	{
		IPlayerComponent* p = m_vAllComponents[i];
		if (p)
		{
			p->onPlayerReconnected();
		}
	}
	saveLoginInfo();
}

void CPlayer::onPlayerLoseConnect()
{
	setState(ePlayerState_Offline);
	for (int i = ePlayerComponent_None; i < ePlayerComponent_Max; ++i)
	{
		IPlayerComponent* p = m_vAllComponents[i];
		if (p)
		{
			p->onPlayerLoseConnect();
		}
	}
}

void CPlayer::onPlayerOtherDeviceLogin(uint32_t nNewSessionID, const char* pNewIP)
{
	setState(ePlayerState_Online);
	// tell client other login
	Json::Value jsMsg;
	sendMsgToClient(jsMsg,MSG_PLAYER_OTHER_LOGIN );

	// tell old gate other logined 
	stMsgClientOtherLogin msg;
	msg.nTargetID = m_nSessionID;
	sendMsgToClient(&msg, sizeof(msg));

	for (int i = ePlayerComponent_None; i < ePlayerComponent_Max; ++i)
	{
		IPlayerComponent* p = m_vAllComponents[i];
		if (p)
		{
			p->onPlayerOtherDeviceLogin(m_nSessionID,nNewSessionID);
		}
	}
	// set new session id ;
	m_nSessionID = nNewSessionID;
	m_strCurIP = pNewIP;
	saveLoginInfo();
}

void CPlayer::onPlayerDisconnect()
{
	// inform components;
	for (int i = ePlayerComponent_None; i < ePlayerComponent_Max; ++i)
	{
		IPlayerComponent* p = m_vAllComponents[i];
		if (p)
		{
			p->onPlayerDisconnect();
		}
	}

	setState(ePlayerState_Offline);
	LOGFMTE("player disconnect should inform other sever");

	if ( canRemovePlayer() )
	{
		onTimerSave();
		getPlayerMgr()->doRemovePlayer(this);
	}
	else
	{
		delayRemove();
	}
}

void CPlayer::delayRemove()
{
	LOGFMTD("player = %u , need to delay remove", getUserUID());
	m_tTimerCheckRemovePlayer.setInterval(5 * 60);
	m_tTimerCheckRemovePlayer.setIsAutoRepeat(true);
	m_tTimerCheckRemovePlayer.setCallBack([this](CTimer* p, float fDelta)
	{
		if (false == canRemovePlayer())
		{
			LOGFMTD("player = %u , need to delay remove go on wait ", getUserUID());
			return;
		}
		LOGFMTD("player = %u , do delay removed", getUserUID());
		onTimerSave();
		m_tTimerCheckRemovePlayer.canncel();
		getPlayerMgr()->doRemovePlayer(this);
	});
	m_tTimerCheckRemovePlayer.start();
}

bool CPlayer::onMsg( stMsg* pMsg , eMsgPort eSenderPort, uint32_t nSenderID )
{
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
	LOGFMTE("Unprocessed json msg id = %d, from = %d  uid = %d",nmsgType,eSenderPort,getUserUID() ) ;
	return false ;
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

void CPlayer::sendMsgToClient( stMsg* pBuffer, uint16_t nLen  )
{
	if ( isState(ePlayerState_Online) )
	{
		if (pBuffer->nTargetID != getSessionID() )
		{
			LOGFMTE("msg type = %u send to client but target id is not sessionid , uid = %u ",pBuffer->cSysIdentifer,getUserUID() );
		}
		m_pPlayerMgr->sendMsg(pBuffer, nLen, getUserUID() );
		return ;
	}
	LOGFMTD("player uid = %d not online so , can not send msg" ,getUserUID() ) ;
}

void CPlayer::sendMsgToClient(Json::Value& jsMsg, uint16_t nMsgType  )
{
	if ( isState(ePlayerState_Offline) == false  )
	{
		m_pPlayerMgr->sendMsg(jsMsg, nMsgType, getUserUID(), getSessionID(), ID_MSG_PORT_CLIENT);
		return ;
	}
	LOGFMTD("player uid = %d not online so , can not send msg" ,getUserUID() ) ;
}

bool CPlayer::isState( ePlayerState eState )
{
	if ( (m_eSate & eState) == eState )
	{
		return true ;
	}
	return false ;
}

void CPlayer::onTimerSave()
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

bool CPlayer::onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)
{
	return false;
}

void CPlayer::saveLoginInfo()
{
	Json::Value jssql;
	char pBuffer[512] = { 0 };
	sprintf_s(pBuffer, "update playerbasedata set loginIP = '%s',loginTime = now() where userUID = %u ;", getIp(), getUserUID() );
	std::string str = pBuffer;
	jssql["sql"] = pBuffer;
	auto pReqQueue = m_pPlayerMgr->getSvrApp()->getAsynReqQueue();
	pReqQueue->pushAsyncRequest(ID_MSG_PORT_DB, getUserUID(), getUserUID(), eAsync_DB_Update, jssql);
}

bool CPlayer::canRemovePlayer()
{
	for (int i = ePlayerComponent_None; i < ePlayerComponent_Max; ++i)
	{
		IPlayerComponent* p = m_vAllComponents[i];
		if (p && p->canRemovePlayer() == false )
		{
			return false;
		}
	}
	return true;
}

