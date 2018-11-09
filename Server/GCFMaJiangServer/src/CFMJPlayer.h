#pragma once
#include "IMJPlayer.h"
#include "CFMJPlayerCard.h"
class CFMJPlayer
	:public IMJPlayer
{
public:
	IMJPlayerCard* getPlayerCard()override;
	void onGameWillStart()override;
	void addExtraTime(float fTime);
	float getExtraTime() { return m_nExtraTime; }
	void init(stEnterRoomData* pData, uint16_t nIdx)override;
	void clearGangFlag() { clearFlag(eMJActFlag_Gang); }
	bool haveGangFlag() { return haveFlag(eMJActFlag_Gang); }
protected:
	CFMJPlayerCard m_tPlayerCard;
	float m_nExtraTime;
};