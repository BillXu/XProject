#pragma once
#include "IGamePlayer.h"
#include "NiuNiu\NiuNiuPeerCard.h"
class NNPlayer
	:public IGamePlayer
{
public:
	NNPlayer() { m_nLastOffset = 0; m_nRobotBankerFailTimes = 0; m_isRobotBanker = false; m_isTuoGuan = false; m_isLastTuiZhu = false; }
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
	bool isRobotBanker() { return m_isRobotBanker; }
	bool recorderVisitor(std::shared_ptr<IPlayerRecorder> ptrPlayerReocrder)override;
	bool isTuoGuan();
	void setTuoGuanFlag(uint8_t isTuoGuan);
	void setIsOnline(bool isOnline)override;
	bool isLastTuiZhu() { return m_isLastTuiZhu; }
	void setLastTuiZhu(bool isTuiZhu) { m_isLastTuiZhu = isTuiZhu; }
protected:
	CNiuNiuPeerCard m_tPeerCard;
	bool m_isCaculatedNiu;
	uint8_t m_nBetTimes;
	uint8_t m_nRobotBankerTimes = 0;
	int8_t m_nLastOffset;
	bool m_isLastTuiZhu; 
	uint8_t m_nRobotBankerFailTimes;
	bool m_isRobotBanker;
	bool m_isTuoGuan;
};