#pragma once
#include "IMJOpts.h"
class SDMJOpts
	:public IMJOpts
{
public:
	void initRoomOpts(Json::Value& jsOpts)override;
	uint16_t calculateDiamondNeed()override;
	uint8_t calculateInitRound()override;
	uint8_t getRuleMode() { return m_nRuleMode; }
	bool isEnableHHQD() { return m_bEnableHaoHuaQiDui && m_bEnableQiDui; }
	bool isEnableDiLing() { return m_bEnableDiLing; }
	bool isEnableQiDui() { return m_bEnableQiDui; }
	bool isEnableOnlyZiMo() { return m_bEnableOnlyZiMo; }

private:
	uint8_t m_nRuleMode;
	bool m_bEnableHaoHuaQiDui;
	bool m_bEnableDiLing;
	bool m_bEnableQiDui;
	bool m_bEnableOnlyZiMo;
};