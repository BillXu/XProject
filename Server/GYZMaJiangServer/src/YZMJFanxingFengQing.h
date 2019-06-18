#pragma once
#include "IFanxing.h"
class YZMJFanxingFengQing
	:public IFanxing
{
	uint16_t getFanxingType()override { return eFanxing_FengQing; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override;
};