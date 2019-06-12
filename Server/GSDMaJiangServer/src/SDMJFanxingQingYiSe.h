#pragma once
#include "FanxingQingYiSe.h"
#include "SDMJPlayerCard.h"
class SDMJFanxingQingYiSe
	:public FanxingQingYiSe
{
public:
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		return ((SDMJPlayerCard*)pPlayerCard)->checkQingYiSe();
	}
};