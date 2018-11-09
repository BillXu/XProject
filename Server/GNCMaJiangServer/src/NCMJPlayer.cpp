#include "NCMJPlayer.h"
void NCMJPlayer::init(stEnterRoomData* pData, uint16_t nIdx) {
	IMJPlayer::init(pData, nIdx);
	m_nExtraTime = 0;
}

IMJPlayerCard* NCMJPlayer::getPlayerCard()
{
	return &m_tPlayerCard;
}

void NCMJPlayer::onGameWillStart() {
	IMJPlayer::onGameWillStart();
	setRace();
	clearGangHouPao();
}

void NCMJPlayer::addExtraTime(float fTime) {
	m_nExtraTime += fTime;
}