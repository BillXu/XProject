#include "SCMJPlayer.h"
IMJPlayerCard* SCMJPlayer::getPlayerCard()
{
	return &m_tPlayerCard;
}

void SCMJPlayer::onGameWillStart()
{
	IMJPlayer::onGameWillStart();
	getPlayerCard()->reset();
}