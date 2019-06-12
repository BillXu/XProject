#include "NCMJPlayer.h"
void NCMJPlayer::init(IGameRoom* pRoom, stEnterRoomData* pData, uint16_t nIdx) {
	IMJPlayer::init(pRoom, pData, nIdx);
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