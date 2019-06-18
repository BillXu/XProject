#pragma once
#include "FanxingDuiDuiHu.h"
class YZMJFanxingDuiDuiHu
	:public FanxingDuiDuiHu
{
public:
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override;
};