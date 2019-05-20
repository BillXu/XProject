#pragma once
#include "FanxingHaiDiLaoYue.h"
class DDMJFanxingHaiDiLaoYue
	:public FanxingHaiDiLaoYue
{
public:
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override;
};