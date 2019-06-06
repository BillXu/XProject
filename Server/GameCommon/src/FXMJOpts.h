#pragma once
#include "IMJOpts.h"
class FXMJOpts
	:public IMJOpts
{
public:
	void initRoomOpts(Json::Value& jsOpts)override;
	uint16_t calculateDiamondNeed()override;
	uint8_t calculateInitRound()override;

	uint8_t getFanLimit() { return m_nFanLimit; }
	bool isDianPaoOnePay() { return m_bDianPaoOnePay; }
	uint32_t getGuang() { return m_nGuang; }
	bool isEnable7Pair() { return m_bEnable7Pair; }
	bool isEnableOnlyOneType() { return m_bEnableOnlyOneType; }
	bool isEnableSB1() { return m_bEnableSB1; }
	bool isEnableFollow() { return m_bEnableFollow; }
	bool isEnableZha5() { return m_bEnableZha5; }
	bool isEnableCool() { return m_bEnableCool; }
	bool isEnablePJH() { return m_bEnablePJH; }

private:
	uint8_t m_nFanLimit;
	bool m_bDianPaoOnePay;
	uint32_t m_nGuang;
	bool m_bEnable7Pair;
	bool m_bEnableOnlyOneType;
	bool m_bEnableSB1;
	bool m_bEnableFollow;
	bool m_bEnableZha5;
	bool m_bEnableCool;
	bool m_bEnablePJH;
};