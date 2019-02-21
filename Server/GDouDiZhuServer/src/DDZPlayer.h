#pragma once
#include "IGamePlayer.h"
#include "DDZPlayerCard.h"
class DDZPlayer
	:public IGamePlayer
{
public:
	void init(stEnterRoomData* pData, uint16_t nIdx)override;
	DDZPlayer() { m_isChaoZhuang = false; }
	void onGameWillStart()override;
	void onGameDidEnd()override;
	DDZPlayerCard* getPlayerCard();
	bool isMingPai();
	void doMingPai();
	bool isTuoGuan();
	void setTuoGuanFlag( uint8_t isTuoGuan );
	void doChaoZhuang();
	bool isChaoZhuang();
	void doTiLaChuai();
	bool isTiLaChuai();

	void signDouble(uint8_t nDouble = 0) { m_bDouble = nDouble; }
	void clearDouble() { m_bDouble = 1; }
	bool isDoubleDone() { return m_bDouble != 1; }
	uint8_t getDouble() { return m_bDouble; }
	void addExtraTime(float fTime);
	float getExtraTime() { return m_nExtraTime; }
protected:
	DDZPlayerCard m_tPeerCard;
	bool m_isChaoZhuang;

	uint8_t m_bDouble;
	float m_nExtraTime;
};