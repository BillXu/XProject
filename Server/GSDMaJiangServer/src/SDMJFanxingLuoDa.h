#pragma once
#include "IFanxing.h"
#include "SDMJPlayerCard.h"
class SDMJFanxingLuoDa
	:public IFanxing
{
	uint16_t getFanxingType()override { return eFanXing_SD_LuoDa; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		return ((SDMJPlayerCard*)pPlayerCard)->isHaveDa() == 0;
	}
};