#pragma once
#include "IGamePlayer.h"
#include "Timer.h"
class IMJPlayerCard;
class IMJPlayer
	:public IGamePlayer
{
public:
	enum eMJActFlag
	{
		eMJActFlag_Gang = 1 ,
		eMJActFlag_DeclBuGang = 1 << 1 ,
		eMJActFlag_BuGang = 1 << 2 | eMJActFlag_Gang,
		eMJActFlag_AnGang = 1 << 3 | eMJActFlag_Gang,
		eMJActFlag_MingGang = 1 << 4 | eMJActFlag_Gang,

		eMJActFlag_LouHu = 1 << 5,
		eMJActFlag_LouPeng = 1 << 6,

		eMJActFlag_CanTianHu = 1 << 7,
		eMJActFlag_WaitCheckTianTing = 1 << 8,
		eMJActFlag_TianTing = 1 << 9,
		eMJActFlag_Max,
	};
public:
	void init(stEnterRoomData* pData, uint16_t nIdx )override;
	void onGameWillStart()override;
	void onGameStart()override;
	void onGameDidEnd()override;
	void onGameEnd()override;
	
	void signFlag( uint32_t nFlag );
	void clearFlag( uint32_t nFlag );
	bool haveFlag( uint32_t nFlag );
	void zeroFlag();

	uint8_t getDianPaoCnt();
	void addDianPaoCnt();

	uint8_t getHuCnt();
	void addHuCnt();

	uint8_t getZiMoCnt();
	void addZiMoCnt();

	uint8_t getAnGangCnt();
	void addAnGangCnt();

	uint8_t getMingGangCnt();
	void addMingGangCnt();
	virtual IMJPlayerCard* getPlayerCard() = 0 ;
private:
	uint32_t m_nFlag;
	uint8_t m_nHuCnt;
	uint8_t m_nZiMoCnt; 
	uint8_t m_nDianPaoCnt;
	uint8_t m_nMingGangCnt;
	uint8_t m_nAnGangCnt;
};