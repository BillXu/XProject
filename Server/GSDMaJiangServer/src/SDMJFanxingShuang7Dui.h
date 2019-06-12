#pragma once
#include "FanxingShuang7Dui.h"
#include "SDMJPlayerCard.h"
class SDMJFanxingShuang7Dui
	:public FanxingShuang7Dui
{
public:
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		return ((SDMJPlayerCard*)pPlayerCard)->checkHaoHuaQiDui();
	}
};