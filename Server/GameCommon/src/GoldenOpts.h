#pragma once
#include "IPokerOpts.h"
class GoldenOpts
	:public IPokerOpts
{
public:
	void initRoomOpts(Json::Value& jsOpts)override;
	uint16_t calculateDiamondNeed()override;
	uint8_t calculateInitRound()override;

	uint8_t getBaseScore() { return m_nBaseScore; }
	uint8_t getMultiple() { return m_nMultiple; }
	uint8_t getMustMenCircle() { return m_nMustMenCircle; }
	uint8_t getCanPKCircle() { return m_nCanPKCircle; }
	uint8_t getCircleLimit() { return m_nCircleLimit; }
	uint8_t getPKTimes() { return m_nPKTimes; }
	uint16_t getWaitActTime() { return m_nWaitActTime; }
	bool isEnable235() { return m_bEnable235; }
	bool isEnableStraight() { return m_bEnableStraight; }
	bool isEnableXiQian() { return m_bEnableXiQian; }

private:
	uint8_t m_nBaseScore;
	uint8_t m_nMultiple;
	uint8_t m_nMustMenCircle;
	uint8_t m_nCanPKCircle;
	uint8_t m_nCircleLimit;
	uint8_t m_nPKTimes;
	uint16_t m_nWaitActTime;
	bool m_bEnable235;
	bool m_bEnableStraight;
	bool m_bEnableXiQian;
};