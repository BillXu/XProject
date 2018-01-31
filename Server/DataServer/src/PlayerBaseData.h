#pragma once
#include "MessageDefine.h"
#include "CommonDefine.h"
#include "IPlayerComponent.h"
#include <string>
struct stMsg ;
class CPlayer ;
struct stEventArg ;
class CPlayerBaseData 
	:public IPlayerComponent
{
public:
	CPlayerBaseData(CPlayer*);
	~CPlayerBaseData();
	void init()override;
	void reset()override;
	void onPlayerLogined()override;
	void onPlayerOtherDeviceLogin(uint32_t nOldSessionID, uint32_t nNewSessionID)override;
	bool onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort)override;
	bool onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID)override;
	void sendBaseDataToClient();
	void saveMoney();
	void saveClub();
	void timerSave()override;
	uint32_t getCoin(){ return m_stBaseData.nCoin ; }
	uint32_t getDiamoned(){ return m_stBaseData.nDiamoned;}
	uint32_t getEmojiCnt() { return m_stBaseData.nEmojiCnt; }
	bool modifyEmojiCnt(int32_t nOffset );
	bool modifyMoney(int32_t nOffset,bool bDiamond = false );
	const char* getPlayerName(){ return m_stBaseData.cName ;}
	const char* getHeadIcon() { return m_stBaseData.cHeadiconUrl; }
	uint8_t getSex(){ return m_stBaseData.nSex ;}
	bool isPlayerReady()override { return m_isReadingDB == false; }
	double getGPS_J() { return m_stBaseData.dfJ; }
	double getGPS_W() { return m_stBaseData.dfW; }
	void getJoinedAndCreatedClubs(std::vector<uint32_t>& vClubIDs);

	void addJoinedClub(uint32_t nClubID);
	void addCreatedClub(uint32_t nClubID);
	void removeJoinedClub(uint32_t nClubID);
	void removeCreatedClub(uint32_t nClubID);
	void dismissClub(uint32_t nClubID);

private:
	stServerBaseData m_stBaseData ;
	bool m_bMoneyDataDirty;
	bool m_bPlayerInfoDirty;
	bool m_bClubDataDirty;
	bool m_isReadingDB;

	int32_t m_nTmpCoin;
	int32_t m_nTmpDiamond;
};