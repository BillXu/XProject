#include "SDMJPlayer.h"
void SDMJPlayer::init(IGameRoom* pRoom, stEnterRoomData* pData, uint16_t nIdx) {
	IMJPlayer::init(pRoom, pData, nIdx);
	m_jsBestCards.clear();
	m_nBestFan = -1;
	m_nExtraTime = 0;
}

IMJPlayerCard* SDMJPlayer::getPlayerCard()
{
	return &m_tPlayerCard;
}

void SDMJPlayer::onGameWillStart()
{
	IMJPlayer::onGameWillStart();
	getPlayerCard()->reset();
	signFlag(eMJActFlag_CanCyclone);
	clearFlag(eMJActFlag_NeedClearCanCyclone);
}

uint32_t SDMJPlayer::addGuangSingleOffset(int32_t nOffset, uint32_t nMaxOffset) {
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

bool SDMJPlayer::canBackGain(uint32_t nMaxOffset) {
	if (nMaxOffset && getChips() < 0) {
		return ((uint32_t)abs(getChips())) < nMaxOffset;
	}
	return true;
}

void SDMJPlayer::setBestCards(uint16_t nFan) {
	if (uint16_t(-1) == m_nBestFan || m_nBestFan < nFan) {
		m_nBestFan = nFan;
		((SDMJPlayerCard*)getPlayerCard())->onVisitPlayerCardBaseInfo(m_jsBestCards);
	}
}

void SDMJPlayer::addExtraTime(float fTime) {
	m_nExtraTime += fTime;
}