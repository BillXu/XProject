#pragma once
#include "FanxingHunYiSe.h"
#include "SDMJPlayerCard.h"
class SDMJFanxingHunYiSe
	:public FanxingHunYiSe
{
public:
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		return ((SDMJPlayerCard*)pPlayerCard)->checkHunYiSe();
	}
};