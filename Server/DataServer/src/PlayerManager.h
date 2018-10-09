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
class CPlayerBrifDataCacher
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
		Json::Value jsBrifData;
		time_t tRequestDataTime ;
		
		std::map<uint32_t,stSubscriber> vBrifeSubscribers ;
		std::map<uint32_t,stSubscriber> vDetailSubscribers ;
		stPlayerDataPrifle() { nPlayerUID = 0; jsBrifData.clear(); }
		~stPlayerDataPrifle(){ vBrifeSubscribers.clear() ; vDetailSubscribers.clear() ;}
		bool isContentData(){ return jsBrifData.isNull() == false ; }
		void recivedData( Json::Value& jsData, IServerApp* pApp ) ;
		void addSubscriber( uint32_t nSessionId , bool isDetail );
	};

	typedef std::map<uint32_t,stPlayerDataPrifle*> MAP_ID_DATA;
public:
	~CPlayerBrifDataCacher();
	void init(IServerApp* pApp) { m_pApp = pApp; }
	stPlayerDataPrifle* getBrifData( uint32_t nUID );
	void removePlayerDataCache( uint32_t nUID );
	bool sendPlayerDataProfile( uint32_t nReqUID ,bool isDetail , uint32_t nSubscriberSessionID );
	void visitBrifData(Json::Value& jsBrifData ,CPlayer* pPlayer );
	void checkState();
protected:
	MAP_ID_DATA m_vDetailData ;
	IServerApp* m_pApp;
};

class CPlayerManager
	:public IGlobalModule
{
public:
	typedef std::map<uint32_t, CPlayer*> MAP_UID_PLAYERS ;
	typedef std::list<CPlayer*> LIST_PLAYERS ;
	typedef std::map<uint32_t, std::vector<std::string>> MAP_ID_GATE_IP;
public:
	CPlayerManager();
	~CPlayerManager();
	void init(IServerApp* svrApp)override;
	bool onMsg(stMsg* prealMsg , eMsgPort eSenderPort , uint32_t nSenderID )override ;
	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID)override;
	CPlayer* getPlayerByUserUID( uint32_t nUserUID );
	void update(float fDeta )override ;
	void onExit()override ;
	bool onAsyncRequest( uint16_t nRequestType , const Json::Value& jsReqContent, Json::Value& jsResult )override ;
	bool onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID, uint32_t nTargetID)override;
	void doRemovePlayer(CPlayer* pOfflinePlayer);
	bool onOtherSvrShutDown(eMsgPort nSvrPort, uint16_t nSvrIdx, uint16_t nSvrMaxCnt)override;
	void doPlayerLogin(uint32_t nUID, uint32_t nSessionID = 0, std::string pIP = "0");
	bool initGateIP();
	std::string getGateIP(uint8_t nGateLevel, uint32_t nUserID);
	std::string getSpecialGateIP(uint32_t nUserID);
protected:
	bool onPublicMsg( stMsg* prealMsg , eMsgPort eSenderPort , uint32_t nSessionID );
	void addActivePlayer( CPlayer* pNewPlayer );
	void logState();
	CPlayer* getReserverPlayerObj();
	void onTimeSave()override;
protected:
	// logic data ;
	MAP_UID_PLAYERS m_vAllActivePlayers ;
	LIST_PLAYERS m_vReserverPlayerObj ;
	LIST_PLAYERS m_vWillDeletePlayers;

	CPlayerBrifDataCacher m_tPlayerDataCaher ;
	MAP_ID_GATE_IP m_mGateIP;
};