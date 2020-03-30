#include "NJMJPlayer.h"
void NJMJPlayer::init(IGameRoom* pRoom, stEnterRoomData* pData, uint16_t nIdx) {
	IMJPlayer::init(pRoom, pData, nIdx);
	m_jsBestCards.clear();
	m_nBestFan = -1;
	m_nExtraTime = 0;
	m_nExtraOffset = 0;
}

IMJPlayerCard* NJMJPlayer::getPlayerCard()
{
	return &m_tPlayerCard;
}

void NJMJPlayer::onGameWillStart()
{
	IMJPlayer::onGameWillStart();
	getPlayerCard()->reset();
	signFlag(eMJActFlag_CanCyclone);
	clearFlag(eMJActFlag_NeedClearCanCyclone);
	setSongGangIdx();
}

uint32_t NJMJPlayer::addGuangSingleOffset(int32_t nOffset, uint32_t nMaxOffset) {
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

bool NJMJPlayer::canBackGain(uint32_t nMaxOffset) {
	if (nMaxOffset && getChips() < 0) {
		return ((uint32_t)abs(getChips())) < nMaxOffset;
	}
	return true;
}

bool NJMJPlayer::canPayOffset(uint32_t nOffset, uint32_t nMaxOffset) {
	if (nMaxOffset) {
		int32_t nFinal = getChips() - (int32_t)nOffset;
		if (nFinal < 0) {
			return ((uint32_t)abs(nFinal)) <= nMaxOffset;
		}
	}
	return true;
}

bool NJMJPlayer::isGuangAfterOffset(uint32_t nOffset, uint32_t nMaxOffset) {
	if (nMaxOffset) {
		int32_t nFinal = getChips() - (int32_t)nOffset;
		if (nFinal < 0) {
			return ((uint32_t)abs(nFinal)) >= nMaxOffset;
		}
	}
	return false;
}

void NJMJPlayer::setBestCards(uint16_t nFan) {
	if (uint16_t(-1) == m_nBestFan || m_nBestFan < nFan) {
		m_nBestFan = nFan;
		((NJMJPlayerCard*)getPlayerCard())->onVisitPlayerCardBaseInfo(m_jsBestCards);
	}
}

void NJMJPlayer::addExtraTime(float fTime) {
	m_nExtraTime += fTime;
}

void NJMJPlayer::setSongGangIdx(uint8_t nIdx) {
	if ((uint8_t)-1 == nIdx) {
		m_nSongGangIdx = -1;
	}
	else if ((uint8_t)-1 == m_nSongGangIdx) {
		m_nSongGangIdx = nIdx;
	}
}