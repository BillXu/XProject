#pragma once
#include "FanxingHunYiSe.h"
class YZMJFanxingHunYiSe
	:public FanxingHunYiSe
{
public:
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override;
};