#pragma once
#include "IMJOpts.h"
class DDMJOpts
	:public IMJOpts
{
public:
	void initRoomOpts(Json::Value& jsOpts)override;
	uint16_t calculateDiamondNeed()override;
	uint8_t calculateInitRound()override;
	bool isRandomChangeSeat() { return m_bRandomChangeSeat; }
	uint8_t getFanLimit() { return m_nFanLimit; }
	bool isDianPaoOnePay() { return m_bDianPaoOnePay; }
	uint32_t getGuang() { return m_nGuang; }

private:
	bool m_bRandomChangeSeat;
	uint8_t m_nFanLimit;
	bool m_bDianPaoOnePay;
	uint32_t m_nGuang;
};