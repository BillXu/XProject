#pragma once
#include "IFanxing.h"
class YZMJFanxingYiTiaoLong
	:public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_YiTiaoLong; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override;
};