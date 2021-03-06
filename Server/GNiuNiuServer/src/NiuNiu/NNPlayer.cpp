#include "NNPlayer.h"
#include "NiuNiu\NiuNiuPlayerRecorder.h"
void NNPlayer::onGameWillStart()
{
	IGamePlayer::onGameWillStart();
	m_tPeerCard.reset();
	m_isCaculatedNiu = false;
	m_nBetTimes = 0;
	m_nRobotBankerTimes = 0;
	m_isRobotBanker = false;
}

void NNPlayer::onGameDidEnd()
{
	m_nLastOffset = getSingleOffset();
	IGamePlayer::onGameDidEnd();
	if ( getRobotBankerFailedTimes() >= 3 )
	{
		clearRobotBankerFailedTimes();
	}
}

uint16_t NNPlayer::doBet(uint16_t nBetTimes)
{
	m_nBetTimes = (uint8_t)nBetTimes;
	return m_nBetTimes;
}

uint16_t NNPlayer::getBetTimes()
{
	return m_nBetTimes;
}

uint16_t NNPlayer::doRobotBanker(uint16_t nRobotTimes)
{
	m_nRobotBankerTimes = (uint8_t)nRobotTimes;
	m_isRobotBanker = true;
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

bool NNPlayer::recorderVisitor(std::shared_ptr<IPlayerRecorder> ptrPlayerReocrder)
{
	IGamePlayer::recorderVisitor(ptrPlayerReocrder);

	auto p = (NiuNiuPlayerRecorder*)ptrPlayerReocrder.get();
	p->setBetTimes((uint8_t)getBetTimes());

	std::vector<uint8_t> vHoldCards;
	if (getPlayerCard()->getHoldCards(vHoldCards) == 0)
	{
		return false;
	}

	p->setHoldCards(vHoldCards);
	return true;
}

bool NNPlayer::isTuoGuan()
{
	return m_isTuoGuan ;
}

void NNPlayer::setTuoGuanFlag(uint8_t isTuoGuan)
{
	m_isTuoGuan = isTuoGuan;
}

void NNPlayer::setIsOnline(bool isOnline)
{
	IGamePlayer::setIsOnline(isOnline);
	if (isOnline == false )
	{
		setTuoGuanFlag(true);
	}
}