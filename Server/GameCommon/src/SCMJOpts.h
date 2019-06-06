#pragma once
#include "IMJOpts.h"
class SCMJOpts
	:public IMJOpts
{
public:
	void initRoomOpts(Json::Value& jsOpts)override;
	uint16_t calculateDiamondNeed()override;
	uint8_t calculateInitRound()override;
	bool isEnableBloodFlow() { return m_bEnableBloodFlow; }
	bool isZiMoAddFan() { return m_bZiMoAddFan; }
	bool isDianGangZiMo() { return m_bDianGangZiMo; }
	bool isEnableExchange3Cards() { return m_bEnableExchange3Cards; }
	bool isEnable19() { return m_bEnable19; }
	bool isEnableMenQing() { return m_bEnableMenQing; }
	bool isEnableZZ() { return m_bEnableZZ; }
	bool isEnableTDHu() { return m_bEnableTDHu; }
	uint8_t getFanLimit() { return m_nFanLimit; }

private:
	bool m_bEnableBloodFlow;
	bool m_bZiMoAddFan;
	bool m_bDianGangZiMo;
	bool m_bEnableExchange3Cards;
	bool m_bEnable19;
	bool m_bEnableMenQing;
	bool m_bEnableZZ;
	bool m_bEnableTDHu;
	uint8_t m_nFanLimit;
};