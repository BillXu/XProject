#include "NNPlayer.h"
void NNPlayer::onGameWillStart()
{
	IGamePlayer::onGameWillStart();
	m_tPeerCard.reset();
	m_isCaculatedNiu = false;
	m_nBetTimes = 0;
	m_nRobotBankerTimes = 0;
}

uint16_t NNPlayer::doBet(uint16_t nBetTimes)
{
	m_nBetTimes = nBetTimes;
	return m_nBetTimes;
}

uint16_t NNPlayer::getBetTimes()
{
	return m_nBetTimes;
}

uint16_t NNPlayer::doRobotBanker(uint16_t nRobotTimes)
{
	m_nRobotBankerTimes = nRobotTimes;
	return nRobotTimes;
}

uint16_t NNPlayer::getRobotBankerTimes()
{
	return m_nRobotBankerTimes;
}

void NNPlayer::doCaculatedNiu()
{
	m_isCaculatedNiu = true;
	setState(eRoomPeer_Looked);
}

bool NNPlayer::isCaculatedNiu()
{
	return m_isCaculatedNiu;
}

CNiuNiuPeerCard* NNPlayer::getPlayerCard()
{
	return &m_tPeerCard;
}