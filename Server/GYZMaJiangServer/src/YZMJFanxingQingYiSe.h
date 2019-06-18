#pragma once
#include "FanxingQingYiSe.h"
class YZMJFanxingQingYiSe
	:public FanxingQingYiSe
{
public:
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override;
};