#pragma once
#include "IPokerOpts.h"
class GDOpts
	:public IPokerOpts
{
public:
	void initRoomOpts(Json::Value& jsOpts)override;
	uint16_t calculateDiamondNeed()override;
	uint8_t calculateInitRound()override;
	bool isEnableDouble() { return m_bEnableDouble; }
	uint8_t getBaseScore() { return m_nBaseScore; }
	bool isEnableRandomSeat() { return m_bRandomSeat; }
	bool isEnableRandomDa() { return m_bRandomDa; }

protected:
	bool m_bEnableDouble;
	uint8_t m_nBaseScore;
	bool m_bRandomSeat;
	bool m_bRandomDa;
};