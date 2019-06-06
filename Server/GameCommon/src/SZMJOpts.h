#pragma once
#include "IMJOpts.h"
class SZMJOpts
	:public IMJOpts
{
public:
	void initRoomOpts(Json::Value& jsOpts)override;
	uint16_t calculateDiamondNeed()override;
	uint8_t calculateInitRound()override;
	uint8_t getRuleMode() { return m_nRuleMode; }
	bool isEnableHHQD() { return m_bEnableHaoHuaQiDui; }
	bool isEnableDiLing() { return m_bEnableDiLing; }

private:
	uint8_t m_nRuleMode;
	bool m_bEnableHaoHuaQiDui;
	bool m_bEnableDiLing;
};