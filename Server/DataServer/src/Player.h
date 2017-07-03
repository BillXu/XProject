#pragma once 
#include "IPlayerComponent.h"
#include "Timer.h"
#include "ServerMessageDefine.h"
#include "MessageIdentifer.h"
class CPlayerBaseData ;
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
	void init(uint32_t nUserUID, uint32_t nSessionID, const char* pIP);
	void reset( uint32_t nUserUID, uint32_t nSessionID, const char* pIP ) ; // for reuse the object ;
	bool onMsg( stMsg* pMessage , eMsgPort eSenderPort, uint32_t nSenderID );
	bool onMsg( Json::Value& recvValue , uint16_t nmsgType, eMsgPort eSenderPort, uint32_t nSenderID );
	void onPlayerDisconnect();
	void sendMsgToClient( stMsg* pBuffer, uint16_t nLen  );
	void sendMsgToClient( Json::Value& jsMsg, uint16_t nMsgType );
	uint32_t getUserUID(){ return m_nUserUID ;}
	uint32_t getSessionID(){ return m_nSessionID ;}
	IPlayerComponent* getComponent(ePlayerComponentType eType ){ return m_vAllComponents[eType];}
	CPlayerBaseData* getBaseData(){ return (CPlayerBaseData*)getComponent(ePlayerComponent_BaseData);}
	bool isState( ePlayerState eState ); 
	void setState(ePlayerState eSate ){ m_eSate = eSate ; }
	void onAnotherClientLoginThisPlayer( uint32_t nSessionID, const char* pIP );
	void postPlayerEvent(stPlayerEvetArg* pEventArg );
	void onTimerSave(CTimer* pTimer, float fTimeElaps );
	void onReactive(uint32_t nSessionID );
	static uint8_t getMsgPortByRoomType(uint8_t nType );
	bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult);
protected:
	bool processPublicPlayerMsg( stMsg* pMessage , eMsgPort eSenderPort );
protected:
	unsigned int m_nUserUID ;
	unsigned int m_nSessionID ;  // comunicate with the client ;
	IPlayerComponent* m_vAllComponents[ePlayerComponent_Max] ;
	ePlayerState m_eSate ;
	CTimer m_tTimerSave ;
}; 