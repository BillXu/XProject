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
	void sendBaseDataToClient();
	void saveMoney();
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
	void onLeaveClub( uint32_t nClubID );
	void onJoinClub( uint32_t nClubID );
	void onCreatedClub(uint32_t nClubID);
	bool canRemovePlayer()override;
	void SendGateIP();
	uint8_t getGateLevel();
	void setGateLevel(uint8_t nGateLevel);
	void addGameCnt();
	void addTotalDiamond(int32_t nDiamond);
	uint32_t getPoint();
	bool addPoint(int32_t nOffset);
	void savePointRecord(int32_t nOffset, Json::Value jsDetail);
	void savePoint();
	uint32_t getWithdrawPoint();
	void addPointTotalGameCnt();
	void sendPointInfo();
	uint32_t getVipLevel();
	uint32_t getVipInvalidTime();
	bool isOutVipCreateRoomLimit(uint32_t nRoomCnt);
	void saveVipInfo();
	void sendVipInfo();
	uint8_t changeVip(uint32_t nVipLevel, uint32_t nDay = 0);

protected:
	void calculatePointWithdraw();
	void sortPointWithDraw();
	void withdarwPoint();
	void sortVipInfo();

private:
	stServerBaseData m_stBaseData ;
	std::vector<uint32_t> m_vCreatedClubIDs;
	bool m_bMoneyDataDirty;
	bool m_bPlayerInfoDirty;
	bool m_bPointDataDirty;
	bool m_bVipDataDirty;
	bool m_isReadingDB;

	int32_t m_nTmpCoin;
	int32_t m_nTmpDiamond;
};