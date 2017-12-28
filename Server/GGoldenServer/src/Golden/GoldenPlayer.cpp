#include "GoldenPlayer.h"
#include "Golden\GoldenPlayerRecorder.h"
void GoldenPlayer::onGameWillStart()
{
	IGamePlayer::onGameWillStart();
	m_tPeerCard.reset();
	clearCallToEnd();
	clearEndShow();
	clearHaveXiQian();
}

void GoldenPlayer::onGameDidEnd()
{
	IGamePlayer::onGameDidEnd();
}

CGoldenPeerCard* GoldenPlayer::getPlayerCard()
{
	return &m_tPeerCard;
}

bool GoldenPlayer::recorderVisitor(std::shared_ptr<IPlayerRecorder> ptrPlayerReocrder)
{
	IGamePlayer::recorderVisitor(ptrPlayerReocrder);

	auto p = (GoldenPlayerRecorder*)ptrPlayerReocrder.get();

	std::vector<uint8_t> vHoldCards;
	if (getPlayerCard()->getHoldCards(vHoldCards) == 0)
	{
		return false;
	}

	p->setHoldCards(vHoldCards);
	return true;
}