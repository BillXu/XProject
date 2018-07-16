#include "LuoMJPlayer.h"
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