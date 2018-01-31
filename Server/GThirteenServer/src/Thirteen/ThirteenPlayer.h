#pragma once
#include "IGamePlayer.h"
#include "ThirteenPeerCard.h"
class ThirteenPlayer
	:public IGamePlayer
{
public:
	ThirteenPlayer() {}
	void init(stEnterRoomData* pEnterPlayer, uint16_t nIdx)override;
	void onGameWillStart()override;
	void onGameDidEnd()override;
	ThirteenPeerCard* getPlayerCard();
	bool recorderVisitor(std::shared_ptr<IPlayerRecorder> ptrPlayerReocrder)override;

	bool hasDetermined() { return m_bDetermined; }
	void clearDeterMined() { m_bDetermined = false; }
	void signDetermined() { m_bDetermined = true; }
	bool hasRotBanker() { return m_bRotBanker; }
	void clearRotBanker() { m_bRotBanker = false; }
	void signRotBanker() { m_bRotBanker = true; }
	bool hasShowCards() { return m_bShowCards; }
	void clearShowCards() { m_bShowCards = false; }
	void signShowCards() { m_bShowCards = true; }
protected:
	ThirteenPeerCard m_tPeerCard;
	bool m_bDetermined;
	bool m_bRotBanker;
	bool m_bShowCards;
	float m_fWaitDragIn;
};