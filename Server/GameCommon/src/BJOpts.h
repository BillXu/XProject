#pragma once
#include "IPokerOpts.h"
class BJOpts
	:public IPokerOpts
{
public:
	void initRoomOpts(Json::Value& jsOpts)override;
	uint16_t calculateDiamondNeed()override;
	uint8_t calculateInitRound()override;
	
	uint8_t getRate() { return m_nRate; }
	bool isEnableSanQing() { return m_bEnableSanQing; }
	bool isEnableShunQingDaTou() { return m_bEnableShunQingDaTou; }
	bool isTianJiSaiMa() { return m_bTianJiSaiMa; }
	bool isEnableGiveUp() { return m_bEnableGiveUp; }

private:
	uint8_t m_nRate;
	bool m_bEnableSanQing;
	bool m_bEnableShunQingDaTou;
	bool m_bTianJiSaiMa;
	bool m_bEnableGiveUp;
};