#pragma once
#include "IMJOpts.h"
class CFMJOpts
	:public IMJOpts
{
public:
	void initRoomOpts(Json::Value& jsOpts)override;
	uint16_t calculateDiamondNeed()override;
	uint8_t calculateInitRound()override;
	uint8_t getFanLimit() { return m_nFanLimit; }

private:
	uint8_t m_nFanLimit;
};