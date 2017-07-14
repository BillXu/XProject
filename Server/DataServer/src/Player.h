#pragma once 
#include "IPlayerComponent.h"
#include "Timer.h"
#include "ServerMessageDefine.h"
#include "MessageIdentifer.h"
class CPlayerBaseData ;
class CPlayerManager;
struct stMsg ;
class CPlayer
{
public:
	enum ePlayerState
	{
		ePlayerState_Offline = 1 ,
		ePlayerState_Online = 1 << 1,
		ePlayerState_WaitReconnect = 1 << 2,
		ePlayerState_Max,
	};
public:
	CPlayer();
	~CPlayer();
	void init( CPlayerManager* pPlayerMgr );
	void reset() ; // for reuse the object ;
	void onPlayerLogined( uint32_t nSessionID , uint32_t nUserUID, const char* pIP );
	void onPlayerReconnected( char* pNewIP );
	void onPlayerLoseConnect();  // wait player reconnect ;
	void onPlayerOtherDeviceLogin( uint32_t nNewSessionID , const char* pNewIP );
	void onPlayerDisconnect();  // should offline 
	void delayRemove();
	bool onMsg( stMsg* pMessage , eMsgPort eSenderPort, uint32_t nSenderID );
	bool onMsg( Json::Value& recvValue , uint16_t nmsgType, eMsgPort eSenderPort, uint32_t nSenderID );
	void sendMsgToClient( stMsg* pBuffer, uint16_t nLen  );
	void sendMsgToClient( Json::Value& jsMsg, uint16_t nMsgType );
	uint32_t getUserUID(){ return m_nUserUID ;}
	uint32_t getSessionID(){ return m_nSessionID ;}
	const char* getIp() { return m_strCurIP.c_str(); }
	IPlayerComponent* getComponent(ePlayerComponentType eType ){ return m_vAllComponents[eType];}
	CPlayerBaseData* getBaseData(){ return (CPlayerBaseData*)getComponent(ePlayerComponent_BaseData);}
	bool isState( ePlayerState eState ); 
	void setState(ePlayerState eSate ){ m_eSate = eSate ; }
	void postPlayerEvent(stPlayerEvetArg* pEventArg );
	void onTimerSave();
	bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult);
	CPlayerManager* getPlayerMgr() { return m_pPlayerMgr; }
	bool isPlayerReady();
protected:
	void saveLoginInfo();
	bool canRemovePlayer();
protected:
	uint32_t m_nUserUID ;
	uint32_t m_nSessionID ;  // comunicate with the client ;
	std::string m_strCurIP;
	IPlayerComponent* m_vAllComponents[ePlayerComponent_Max] ;
	ePlayerState m_eSate ;
	CPlayerManager* m_pPlayerMgr;
	CTimer m_tTimerCheckRemovePlayer;
}; 