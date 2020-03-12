#pragma once
#include "IFanxing.h"
#include "SZMJPlayerCard.h"
class SZMJFanxingDaMenQing
	:public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_DaMenQing; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		return ((SZMJPlayerCard*)pPlayerCard)->checkDaMenQing();
	}
};