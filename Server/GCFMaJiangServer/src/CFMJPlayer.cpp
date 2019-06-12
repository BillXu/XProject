#include "CFMJPlayer.h"
void CFMJPlayer::init(IGameRoom* pRoom, stEnterRoomData* pData, uint16_t nIdx) {
	IMJPlayer::init(pRoom, pData, nIdx);
	m_nExtraTime = 0;
}

IMJPlayerCard* CFMJPlayer::getPlayerCard()
{
	return &m_tPlayerCard;
}

void CFMJPlayer::onGameWillStart() {
	IMJPlayer::onGameWillStart();
	setRace();
}

void CFMJPlayer::addExtraTime(float fTime) {
	m_nExtraTime += fTime;
}