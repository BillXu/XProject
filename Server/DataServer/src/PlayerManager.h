#pragma once 
#include "NetWorkManager.h"
#include "ServerMessageDefine.h"
#include <json/json.h>
#include <map>
#include <list>
#include "IGlobalModule.h"
#include "MessageIdentifer.h"
class CPlayer ;
struct stMsg ;
class CSelectPlayerDataCacher
{
public:
	struct stSubscriber
	{
		uint32_t nSessionID ;
		bool isDetail ;
	};

	struct stPlayerDataPrifle
	{
		uint32_t nPlayerUID ;
		stPlayerDetailData* pData ;
		time_t tRequestDataTime ;
		
		std::map<uint32_t,stSubscriber> vBrifeSubscribers ;
		std::map<uint32_t,stSubscriber> vDetailSubscribers ;
		stPlayerDataPrifle(){ nPlayerUID = 0 ; pData = nullptr ; tRequestDataTime = 0 ; }
		~stPlayerDataPrifle(){ delete pData ; pData = nullptr ; vBrifeSubscribers.clear() ; vDetailSubscribers.clear() ;}
		bool isContentData(){ return pData != nullptr ; }
		void recivedData(stPlayerBrifData* pRecData) ;
		void addSubscriber( uint32_t nSessionId , bool isDetail );
	};

	typedef std::map<uint32_t,stPlayerDataPrifle*> MAP_ID_DATA;
public:
	CSelectPlayerDataCacher();
	~CSelectPlayerDataCacher();
	void removePlayerDataCache( uint32_t nUID );
	void cachePlayerData(stMsgSelectPlayerDataRet* pmsg );
	bool sendPlayerDataProfile(uint32_t nReqUID ,bool isDetail , uint32_t nSubscriberSessionID );
protected:
	MAP_ID_DATA m_vDetailData ;
};

class CPlayerManager
	:public IGlobalModule
{
public:
	typedef std::map<uint32_t, CPlayer*> MAP_UID_PLAYERS ;
	typedef std::list<CPlayer*> LIST_PLAYERS ;
public:
	CPlayerManager();
	~CPlayerManager();
	bool onMsg(stMsg* prealMsg , eMsgPort eSenderPort , uint32_t nSenderID )override ;
	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID)override;
	CPlayer* getPlayerByUserUID( uint32_t nUserUID );
	void update(float fDeta )override ;
	void onExit()override ;
	bool onAsyncRequest( uint16_t nRequestType , const Json::Value& jsReqContent, Json::Value& jsResult )override ;
protected:
	void onPlayerOffline(CPlayer* pOfflinePlayer);
	bool onPublicMsg( stMsg* prealMsg , eMsgPort eSenderPort , uint32_t nSessionID );
	void addPlayer( CPlayer* pNewPlayer );
	void logState();
	CPlayer* getReserverPlayerObj();
	void addToReserverPlayerObj( CPlayer* pPlayer );
public:
	CSelectPlayerDataCacher& getPlayerDataCaher(){ return m_tPlayerDataCaher ;}
protected:
	// logic data ;
	MAP_UID_PLAYERS m_vAllActivePlayers ;
	LIST_PLAYERS m_vReserverPlayerObj ;

	CSelectPlayerDataCacher m_tPlayerDataCaher ;
};