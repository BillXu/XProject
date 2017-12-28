#pragma once
#include "IGamePlayer.h"
#include "Golden\GoldenPeerCard.h"
class GoldenPlayer
	:public IGamePlayer
{
public:
	GoldenPlayer() {}
	void onGameWillStart()override;
	void onGameDidEnd()override;
	CGoldenPeerCard* getPlayerCard();
	bool recorderVisitor(std::shared_ptr<IPlayerRecorder> ptrPlayerReocrder)override;
	bool isCallToEnd() { return m_bCallToEnd; }
	void signCallToEnd() { m_bCallToEnd = true; }
	void clearCallToEnd() { m_bCallToEnd = false; }
	void switchCallToEnd() { m_bCallToEnd = !m_bCallToEnd; }
	bool isEndShow() { return m_bEndShow; }
	void signEndShow() { m_bEndShow = true; }
	void clearEndShow() { m_bEndShow = false; }
	bool haveXiQian() { return m_bHaveXiQian; }
	void signHaveXiQian() { m_bHaveXiQian = true; }
	void clearHaveXiQian() { m_bHaveXiQian = false; }
protected:
	CGoldenPeerCard m_tPeerCard;

	bool m_bCallToEnd;
	bool m_bEndShow;
	bool m_bHaveXiQian;
};