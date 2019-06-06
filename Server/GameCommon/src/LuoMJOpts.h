#pragma once
#include "IMJOpts.h"
class LuoMJOpts
	:public IMJOpts
{
public:
	void initRoomOpts(Json::Value& jsOpts)override;
	uint16_t calculateDiamondNeed()override;
	uint8_t calculateInitRound()override;

	uint8_t getFanLimit() { return m_nFanLimit; }
	bool isDianPaoOnePay() { return m_bDianPaoOnePay; }
	uint32_t getGuang() { return m_nGuang; }
	bool isEnableSB1() { return m_bEnableSB1; }
	bool isEnableCaiGang() { return m_bEnableCaiGang; }
	bool isEnableHunPiao() { return m_bEnableHunPiao; }

private:
	uint8_t m_nFanLimit;
	bool m_bDianPaoOnePay;
	uint32_t m_nGuang;
	bool m_bEnableSB1;
	bool m_bEnableCaiGang;
	bool m_bEnableHunPiao;
};