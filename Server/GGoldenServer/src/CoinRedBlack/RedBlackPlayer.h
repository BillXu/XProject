#pragma once
#include "IGamePlayer.h"
#include <list>
class RedBlackPlayer
	:public IGamePlayer
{
public:
	struct stBetRecorder
	{
		int32_t nBetCoin;
		int32_t nOffset;
	};
public:
	bool doBet(int32_t nBetCoin, eBetPool ePool);
	void onGameEnd()override;
	void onGameDidEnd()override;
	int32_t getBetCoin(eBetPool ePool) { return m_vBetedCoin[ePool]; }
	int32_t getBetCoin();
	int8_t getWinTimes();
protected:
	int32_t m_vBetedCoin[eBet_Max];
	std::list<stBetRecorder> m_vRecorders;
};