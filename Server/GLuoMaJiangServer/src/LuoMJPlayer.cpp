#include "LuoMJPlayer.h"
void LuoMJPlayer::init(stEnterRoomData* pData, uint16_t nIdx) {
	IMJPlayer::init(pData, nIdx);
	m_jsBestCards.clear();
	m_nBestFan = -1;
	m_nExtraTime = 0;
}

IMJPlayerCard* LuoMJPlayer::getPlayerCard()
{
	return &m_tPlayerCard;
}

void LuoMJPlayer::onGameWillStart()
{
	IMJPlayer::onGameWillStart();
	getPlayerCard()->reset();
	signFlag(eMJActFlag_CanCyclone);
	clearFlag(eMJActFlag_NeedClearCanCyclone);
}

uint32_t LuoMJPlayer::addGuangSingleOffset(int32_t nOffset, uint32_t nMaxOffset) {
	if (nMaxOffset && nOffset < 0) {
		if (getChips() + nOffset < -1 * (int32_t)(nMaxOffset)) {
			int32_t nRealOffset = -1 * (int32_t)(nMaxOffset) - getChips();
			IMJPlayer::addSingleOffset(nRealOffset);
			return nRealOffset - nOffset;
		}
	}

	IMJPlayer::addSingleOffset(nOffset);
	return 0;
}

bool LuoMJPlayer::canBackGain(uint32_t nMaxOffset) {
	if (nMaxOffset && getChips() < 0) {
		return ((uint32_t)abs(getChips())) < nMaxOffset;
	}
	return true;
}

void LuoMJPlayer::setBestCards(uint16_t nFan) {
	if (uint16_t(-1) == m_nBestFan || m_nBestFan < nFan) {
		m_nBestFan = nFan;
		((LuoMJPlayerCard*)getPlayerCard())->onVisitPlayerCardBaseInfo(m_jsBestCards);
	}
}

void LuoMJPlayer::addExtraTime(float fTime) {
	m_nExtraTime += fTime;
}