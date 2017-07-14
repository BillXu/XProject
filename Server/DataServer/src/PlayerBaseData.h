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
	void onPlayerOtherDeviceLogin(uint16_t nOldSessionID, uint16_t nNewSessionID)override;
	bool onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort)override;
	void sendBaseDataToClient();
	void saveMoney();
	void timerSave()override;
	uint32_t getCoin(){ return m_stBaseData.nCoin ; }
	uint32_t getDiamoned(){ return m_stBaseData.nDiamoned;}
	bool modifyMoney(int32_t nOffset,bool bDiamond = false );
	const char* getPlayerName(){ return m_stBaseData.cName ;}
	const char* getHeadIcon() { return m_stBaseData.cHeadiconUrl; }
	uint8_t getSex(){ return m_stBaseData.nSex ;}
	bool isPlayerReady()override { return m_isReadingDB == false; }
private:
	stServerBaseData m_stBaseData ;
	bool m_bMoneyDataDirty;
	bool m_bPlayerInfoDirty;
	bool m_isReadingDB;
};