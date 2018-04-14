#include "ThirteenPlayer.h"
#include "ThirteenPeerCard.h"
#include "ThirteenPlayerRecorder.h"
void ThirteenPlayer::init(stEnterRoomData* pEnterPlayer, uint16_t nIdx) {
	IGamePlayer::init(pEnterPlayer, nIdx);
	setChips(pEnterPlayer->nChip);
	clearWaitDrgInTime();
	clearAutoStandUp();
	clearAutoLeave();
}

void ThirteenPlayer::onGameWillStart()
{
	IGamePlayer::onGameWillStart();
	m_tPeerCard.reset();
	clearDeterMined();
	clearRotBanker();
	clearShowCards();
}

void ThirteenPlayer::onGameEnd()
{
	if (haveState(eRoomPeer_WaitDragIn)) {
		return;
	}
	IGamePlayer::onGameEnd();
	/*if (false == haveState(eRoomPeer_Ready))
	{
		setState(eRoomPeer_WaitNextGame);
	}*/
}

void ThirteenPlayer::onGameDidEnd()
{
	if (haveState(eRoomPeer_WaitDragIn)) {
		return;
	}
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

	std::vector<uint8_t> vHold, vTemp, vType;
	for (uint8_t nIdx = DAO_HEAD; nIdx < DAO_MAX; ++nIdx)
	{
		uint8_t nType = 0;
		getPlayerCard()->getGroupInfo(nIdx, nType, vTemp);
		vHold.insert(vHold.end(), vTemp.begin(), vTemp.end());
		vTemp.clear();
		vType.push_back(getPlayerCard()->getType(nIdx));
	}
	pRecorder->setHoldCards(vHold);
	pRecorder->setTypes(vType);
	return true;
}

/*
	return real offset value by abs(uint)
*/
int32_t ThirteenPlayer::addSingleOffset(int32_t nOffset, bool canBeMinus) {
	if (canBeMinus == false && nOffset < 0 && getChips() < -1 * nOffset) {
		int32_t nReal = getChips() > 0 ? -1 * getChips() : 0;
		IGamePlayer::addSingleOffset(nReal);
		return -1 * nReal;
	}
	else {
		IGamePlayer::addSingleOffset(nOffset);
		return nOffset < 0 ? -1 * nOffset : nOffset;
	}
}