#include "AHMJPlayer.h"
void AHMJPlayer::init(IGameRoom* pRoom, stEnterRoomData* pData, uint16_t nIdx) {
	IMJPlayer::init(pRoom, pData, nIdx);
	m_nExtraTime = 0;
}

IMJPlayerCard* AHMJPlayer::getPlayerCard()
{
	return &m_tPlayerCard;
}

void AHMJPlayer::onGameWillStart() {
	IMJPlayer::onGameWillStart();
	setRace();
}

void AHMJPlayer::addExtraTime(float fTime) {
	m_nExtraTime += fTime;
}