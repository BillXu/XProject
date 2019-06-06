#pragma once
#include "IMJOpts.h"
class AHMJOpts
	:public IMJOpts
{
public:
	void initRoomOpts(Json::Value& jsOpts)override;
	uint16_t calculateDiamondNeed()override;
	uint8_t calculateInitRound()override;

	uint8_t getFanLimit() { return m_nFanLimit; }
	bool isEnableRace() { return m_bEnableRace; }

private:
	uint8_t m_nFanLimit;
	bool m_bEnableRace;
};