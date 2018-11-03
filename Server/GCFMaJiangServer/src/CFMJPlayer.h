#pragma once
#include "IMJPlayer.h"
#include "CFMJPlayerCard.h"
class CFMJPlayer
	:public IMJPlayer
{
public:
	IMJPlayerCard* getPlayerCard()override;
	void onGameWillStart()override;
	void clearLouHu();
	void setLouHuCnt(uint8_t louHuCnt) { m_nLouHuCnt = louHuCnt; }
	uint8_t getLouHuCnt() { return m_nLouHuCnt; }
protected:
	CFMJPlayerCard m_tPlayerCard;
	uint8_t m_nLouHuCnt = 0;
};