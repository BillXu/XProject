#pragma once
#include "ServerMessageDefine.h"
#include <json/json.h>
class CPlayer ;

enum ePlayerComponentType
{
	ePlayerComponent_None ,
	ePlayerComponent_BaseData,
	ePlayerComponent_PlayerGameData ,
	ePlayerComponent_Mail,            // last sit the last pos ,
	ePlayerComponent_Max,
};

struct stPlayerEvetArg ;
class IPlayerComponent
{
public:
	IPlayerComponent(CPlayer* pPlayer );
	virtual ~IPlayerComponent();
	virtual void reset() {}
	virtual void init() { }
	void sendMsg(stMsg* pbuffer , unsigned short nLen );
	void sendMsg(Json::Value& jsMsg , uint16_t nMsgType );
	ePlayerComponentType getComponentType(){ return m_eType ;}
	CPlayer* getPlayer(){ return m_pPlayer ;}
	virtual bool onMsg( stMsg* pMessage , eMsgPort eSenderPort);
	virtual bool onMsg( Json::Value& recvValue , uint16_t nmsgType, eMsgPort eSenderPort ){ return false ;}
	virtual bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult) { return false; }
	virtual void onPlayerDisconnect() { timerSave(); }
	virtual void onPlayerReconnected(){}
	virtual void onPlayerLoseConnect() { timerSave(); }  // wait player reconnect ;
	virtual void onPlayerOtherDeviceLogin( uint16_t nOldSessionID, uint16_t nNewSessionID  ){}
	virtual void onPlayerLogined(){}

	virtual bool onPlayerEvent(stPlayerEvetArg* pArg){ return false ; }
	virtual void timerSave(){};
	virtual bool canRemovePlayer() { return true; }
	virtual bool isPlayerReady() { return true; }
protected:
	CPlayer* m_pPlayer ;
	ePlayerComponentType m_eType ;	
};