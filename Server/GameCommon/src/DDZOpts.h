#pragma once
#include "IPokerOpts.h"
class DDZOpts
	:public IPokerOpts
{
public:
	void initRoomOpts(Json::Value& jsOpts)override;
	uint16_t calculateDiamondNeed()override;
	uint8_t calculateInitRound()override;
	uint32_t getFengDing() { return m_nFengDing; }
	uint8_t getRLT() { return m_nRotLandlordType; }
	bool isEnableDouble() { return m_bEnableDouble; }
	uint8_t getBaseScore() { return m_nBaseScore; }
	uint32_t getFanLimit() { return m_nFanLimit; }
	bool isNotShuffle() { return m_bNotShuffle; }
	bool isEnableChaoZhuang() { return m_bEnableChaoZhuang; }

private:
	uint32_t m_nFengDing;
	uint8_t m_nRotLandlordType;
	bool m_bEnableDouble;
	uint8_t m_nBaseScore;
	uint32_t m_nFanLimit;
	bool m_bNotShuffle;
	bool m_bEnableChaoZhuang;
};