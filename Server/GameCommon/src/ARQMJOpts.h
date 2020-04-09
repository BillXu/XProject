#pragma once
#include "IMJOpts.h"
class ARQMJOpts
	:public IMJOpts
{
public:
	void initRoomOpts(Json::Value& jsOpts)override;
	uint16_t calculateDiamondNeed()override;
	uint8_t calculateInitRound()override;

	uint8_t getFanLimit() { return m_nFanLimit; }
	uint32_t getGuang() { return m_nGuang; }
	bool isEnablePao() { return m_bPao; }

protected:
	uint8_t m_nFanLimit;
	uint32_t m_nGuang;
	bool m_bPao;
};