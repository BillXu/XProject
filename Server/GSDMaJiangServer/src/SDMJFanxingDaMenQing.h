#pragma once
#include "IFanxing.h"
#include "SDMJPlayerCard.h"
class SDMJFanxingDaMenQing
	:public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_DaMenQing; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		return ((SDMJPlayerCard*)pPlayerCard)->checkDaMenQing();
	}
};