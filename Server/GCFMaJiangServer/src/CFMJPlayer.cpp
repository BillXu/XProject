#include "CFMJPlayer.h"
IMJPlayerCard* CFMJPlayer::getPlayerCard()
{
	return &m_tPlayerCard;
}

void CFMJPlayer::onGameWillStart() {
	IMJPlayer::onGameWillStart();
	setRace();
	clearLouHu();
}

void CFMJPlayer::clearLouHu() {
	m_nLouHuCnt = 0;
}