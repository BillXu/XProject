#pragma once
#include "IGlobalModule.h"
class Club; 
class ClubManager
	:public IGlobalModule
{
public:
	typedef std::map<uint32_t, Club*> MAP_CLUB;
public:
	ClubManager() { m_vClubs.clear(); m_nMaxClubID = 0; }
	~ClubManager();
	bool onMsg( Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSenderID, uint32_t nTargetID )override;
	bool onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult)override;
	void onConnectedSvr( bool isReconnected )override;
	void onTimeSave()override;
	void update(float fDeta);
	bool isNameDuplicate( std::string& strName );
protected:
	void readClubs(uint32_t nAlreadyReadCnt);
	void finishReadClubs();
	bool canPlayerCreateClub( uint32_t nUID );
	uint32_t generateClubID();
protected:
	MAP_CLUB m_vClubs;
	uint32_t m_nMaxClubID;
};