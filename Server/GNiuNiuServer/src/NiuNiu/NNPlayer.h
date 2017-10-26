#pragma once
#include "IGamePlayer.h"
#include "NiuNiu\NiuNiuPeerCard.h"
class NNPlayer
	:public IGamePlayer
{
public:
	NNPlayer() { m_nLastOffset = 0; m_nRobotBankerFailTimes = 0; }
	void onGameWillStart()override;
	void onGameDidEnd()override;
	uint16_t doBet( uint16_t nBetTimes );
	uint16_t getBetTimes();
	uint16_t doRobotBanker(uint16_t nRobotTimes);
	uint16_t getRobotBankerTimes();
	void doCaculatedNiu();
	bool isCaculatedNiu();
	CNiuNiuPeerCard* getPlayerCard();
	int32_t getLastOffset() { return m_nLastOffset; }
	void onRobotBankerFailed() { ++m_nRobotBankerFailTimes; }
	void clearRobotBankerFailedTimes() { m_nRobotBankerFailTimes = 0; }
	uint8_t getRobotBankerFailedTimes() { return m_nRobotBankerFailTimes; }
	bool recorderVisitor(std::shared_ptr<IPlayerRecorder> ptrPlayerReocrder)override;
protected:
	CNiuNiuPeerCard m_tPeerCard;
	bool m_isCaculatedNiu;
	uint8_t m_nBetTimes;
	uint8_t m_nRobotBankerTimes = 0;
	int8_t m_nLastOffset;
	uint8_t m_nRobotBankerFailTimes;
};