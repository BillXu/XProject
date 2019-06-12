#pragma once
#include "Fanxing7Dui.h"
#include "SDMJPlayerCard.h"
class SDMJFanxing7Dui
	:public Fanxing7Dui
{
public:
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		return ((SDMJPlayerCard*)pPlayerCard)->checkQiDui();
	}
};