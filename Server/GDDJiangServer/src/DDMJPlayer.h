#pragma once
#include "IMJPlayer.h"
#include "DDMJPlayerCard.h"
class DDMJPlayer
	:public IMJPlayer
{
public:
	IMJPlayerCard* getPlayerCard()override;
	void init(IGameRoom* pRoom, stEnterRoomData* pData, uint16_t nIdx)override;
	void onGameWillStart()override;
	void clearGangFlag() { clearFlag(eMJActFlag_Gang); }
	bool haveGangFlag() { return haveFlag(eMJActFlag_Gang); }
	uint32_t addGuangSingleOffset(int32_t nOffset, uint32_t nMaxOffset = 0);
	bool canBackGain(uint32_t nMaxOffset = 0);
	void setBestCards(uint16_t nFan);
	void setBestChips(uint32_t nBestChips);
	Json::Value getBestCards() { return m_jsBestCards; }
	uint16_t getBestFan() { return m_nBestFan; }
	uint32_t getBestChips() { return m_nBestChips; }
	void addExtraTime(float fTime);
	float getExtraTime() { return m_nExtraTime; }
protected:
	DDMJPlayerCard m_tPlayerCard;

	Json::Value m_jsBestCards;
	uint16_t m_nBestFan;
	uint32_t m_nBestChips;
	float m_nExtraTime;
};