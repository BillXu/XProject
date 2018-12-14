#pragma once
#include "IMJPlayer.h"
#include "FXMJPlayerCard.h"
class FXMJPlayer
	:public IMJPlayer
{
public:
	void init(stEnterRoomData* pData, uint16_t nIdx)override;
	IMJPlayerCard* getPlayerCard()override;
	void onGameWillStart()override;
	void clearGangFlag() { clearFlag(eMJActFlag_Gang); }
	bool haveGangFlag() { return haveFlag(eMJActFlag_Gang); }
	uint32_t addGuangSingleOffset(int32_t nOffset, uint32_t nMaxOffset = 0);
	bool canBackGain(uint32_t nMaxOffset = 0);
	void signEnable7Pair() { m_tPlayerCard.signEnable7Pair(); }
	void signEnableOOT() { m_tPlayerCard.signEnableOOT(); }
	void signEnableSB1() { m_tPlayerCard.signEnableSB1(); }

	void addCoolCnt() { m_nCoolCnt++; }
	uint8_t getCoolCnt() { return m_nCoolCnt; }
protected:
	FXMJPlayerCard m_tPlayerCard;

	uint8_t m_nCoolCnt = 0;
};