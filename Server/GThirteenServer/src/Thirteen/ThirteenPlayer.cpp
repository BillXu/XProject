#include "ThirteenPlayer.h"
#include "ThirteenPeerCard.h"
#include "ThirteenPlayerRecorder.h"
void ThirteenPlayer::onGameWillStart()
{
	IGamePlayer::onGameWillStart();
	m_tPeerCard.reset();
	clearDeterMined();
}

void ThirteenPlayer::onGameDidEnd()
{
	IGamePlayer::onGameDidEnd();
}

ThirteenPeerCard* ThirteenPlayer::getPlayerCard()
{
	return &m_tPeerCard;
}

bool ThirteenPlayer::recorderVisitor(std::shared_ptr<IPlayerRecorder> ptrPlayerReocrder)
{
	IGamePlayer::recorderVisitor(ptrPlayerReocrder);
	auto pRecorder = (ThirteenPlayerRecorder*)ptrPlayerReocrder.get();

	std::vector<uint8_t> vHold, vTemp;
	for (uint8_t nIdx = DAO_HEAD; nIdx < DAO_MAX; ++nIdx)
	{
		uint8_t nType = 0;
		getPlayerCard()->getGroupInfo(nIdx, nType, vTemp);
		vHold.insert(vHold.end(), vTemp.begin(), vTemp.end());
		vTemp.clear();
	}
	pRecorder->setHoldCards(vHold);
	return true;
}