#pragma once
#include "FanxingDuiDuiHu.h"
#include "SDMJPlayerCard.h"
class SDMJFanxingDuiDuiHu
	:public FanxingDuiDuiHu
{
public:
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		return ((SDMJPlayerCard*)pPlayerCard)->checkDuiDuiHu();
	}
};