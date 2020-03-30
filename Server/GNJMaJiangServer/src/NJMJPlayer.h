#pragma once
#include "IMJPlayer.h"
#include "NJMJPlayerCard.h"
class NJMJPlayer
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
	bool canPayOffset(uint32_t nOffset, uint32_t nMaxOffset = 0);
	bool isGuangAfterOffset(uint32_t nOffset, uint32_t nMaxOffset = 0);
	void setBestCards(uint16_t nFan);
	Json::Value getBestCards() { return m_jsBestCards; }
	uint16_t getBestFan() { return m_nBestFan; }
	void addExtraTime(float fTime);
	float getExtraTime() { return m_nExtraTime; }
	void addExtraOffset(int32_t nOffset) { m_nExtraOffset += nOffset; }
	int32_t getExtraOffset() { return m_nExtraOffset; }
	void setSongGangIdx(uint8_t nIdx = -1);
	uint8_t getSongGangIdx() { return m_nSongGangIdx; }
	bool isBaoMi() { return m_bBaoMi; }
	void signBaoMi() { m_bBaoMi = true; }
protected:
	NJMJPlayerCard m_tPlayerCard;
	int32_t m_nExtraOffset;
	uint8_t m_nSongGangIdx;

	Json::Value m_jsBestCards;
	uint16_t m_nBestFan;
	float m_nExtraTime;

	bool m_bBaoMi = false;
};