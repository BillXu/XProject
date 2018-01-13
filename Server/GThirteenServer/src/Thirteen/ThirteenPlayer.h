#pragma once
#include "IGamePlayer.h"
#include "ThirteenPeerCard.h"
class ThirteenPlayer
	:public IGamePlayer
{
public:
	ThirteenPlayer() {}
	void onGameWillStart()override;
	void onGameDidEnd()override;
	ThirteenPeerCard* getPlayerCard();
	bool recorderVisitor(std::shared_ptr<IPlayerRecorder> ptrPlayerReocrder)override;

	bool hasDetermined() { return m_bDetermined; }
	void clearDeterMined() { m_bDetermined = false; }
	void signDetermined() { m_bDetermined = true; }
protected:
	ThirteenPeerCard m_tPeerCard;
	bool m_bDetermined;
};