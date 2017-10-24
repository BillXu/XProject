#include "DDZPlayer.h"
void DDZPlayer::onGameWillStart()
{
	IGamePlayer::onGameWillStart();
	m_isMingPai = false;
	getPlayerCard()->reset();
}

DDZPlayerCard* DDZPlayer::getPlayerCard()
{
	return &m_tPeerCard;
}

bool DDZPlayer::isMingPai()
{
	return m_isMingPai;
}

void DDZPlayer::doMingPai()
{
	m_isMingPai = true;
}
