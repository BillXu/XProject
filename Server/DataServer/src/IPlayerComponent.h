#pragma once
#include "ServerMessageDefine.h"
#include <json/json.h>
class CPlayer ;

enum ePlayerComponentType
{
	ePlayerComponent_None ,
	ePlayerComponent_BaseData,
	//ePlayerComponent_Friend,
	ePlayerComponent_PlayerShop,
	ePlayerComponent_PlayerItemMgr,
	//ePlayerComponent_PlayerMission,
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
	virtual void onPlayerDisconnect(){}
	virtual void onPlayerReconnected(){}
	virtual void onOtherWillLogined(){}
	virtual void onOtherDoLogined(){}
	virtual bool onPlayerEvent(stPlayerEvetArg* pArg){ return false ; }
	virtual void timerSave(){};
	virtual void onReactive(uint32_t nSessionID ){ }
protected:
	CPlayer* m_pPlayer ;
	ePlayerComponentType m_eType ;	
};