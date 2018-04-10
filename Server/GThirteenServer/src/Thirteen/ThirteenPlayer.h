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
	void onGameEnd()override;
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
	void addWaitDragInTime(float fTime) { m_fWaitDragIn += fTime; }
	void clearWaitDrgInTime() { m_fWaitDragIn = 0; }
	float getWaitDragInTime() { return m_fWaitDragIn; }
	int32_t getWaitDragIn() { return (int32_t)m_fWaitDragIn; }
	void signAutoStandUp() { m_bAutoStandUp = true; }
	void clearAutoStandUp() { m_bAutoStandUp = false; }
	bool isAutoStandUp() { return m_bAutoStandUp; }
	void signAutoLeave() { m_bAutoLeave = true; }
	void clearAutoLeave() { m_bAutoLeave = false; }
	bool isAutoLeave() { return m_bAutoLeave; }

	int32_t addSingleOffset(int32_t nOffset, bool canBeMinus = true)override;
protected:
	ThirteenPeerCard m_tPeerCard;
	bool m_bDetermined;
	bool m_bRotBanker;
	bool m_bShowCards;
	bool m_bAutoStandUp;
	bool m_bAutoLeave;
	float m_fWaitDragIn;
};