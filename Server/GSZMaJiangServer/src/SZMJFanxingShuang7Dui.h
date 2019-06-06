#pragma once
#include "FanxingShuang7Dui.h"
class SZMJFanxingShuang7Dui
	:public FanxingShuang7Dui
{
public:
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override;
};