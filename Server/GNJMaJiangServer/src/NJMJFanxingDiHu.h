#pragma once
#include "FanxingDiHu.h"
class NJMJFanxingDiHu
	:public FanxingDiHu
{
public:
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override;
};