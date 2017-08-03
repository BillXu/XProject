#pragma once
#include "IGamePlayer.h"
#include "NiuNiu\NiuNiuPeerCard.h"
class NNPlayer
	:public IGamePlayer
{
public:
	void onGameWillStart()override;
	uint16_t doBet( uint16_t nBetTimes );
	uint16_t getBetTimes();
	uint16_t doRobotBanker(uint16_t nRobotTimes);
	uint16_t getRobotBankerTimes();
	void doCaculatedNiu();
	bool isCaculatedNiu();
	CNiuNiuPeerCard* getPlayerCard();
protected:
	CNiuNiuPeerCard m_tPeerCard;
	bool m_isCaculatedNiu;
	uint8_t m_nBetTimes;
	uint8_t m_nRobotBankerTimes = 0;
};