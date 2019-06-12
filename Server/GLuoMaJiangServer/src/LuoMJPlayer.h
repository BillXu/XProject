#pragma once
#include "IMJPlayer.h"
#include "LuoMJPlayerCard.h"
class LuoMJPlayer
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
	Json::Value getBestCards() { return m_jsBestCards; }
	uint16_t getBestFan() { return m_nBestFan; }

	void signEnableSB1() { m_tPlayerCard.signEnableSB1(); }
	void signEnableHunPiao() { m_tPlayerCard.signEnableHunPiao(); }

	void addExtraTime(float fTime);
	float getExtraTime() { return m_nExtraTime; }
protected:
	LuoMJPlayerCard m_tPlayerCard;

	Json::Value m_jsBestCards;
	uint16_t m_nBestFan;
	float m_nExtraTime;
};