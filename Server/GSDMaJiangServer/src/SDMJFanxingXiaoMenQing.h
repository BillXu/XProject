#pragma once
#include "IFanxing.h"
#include "SDMJPlayerCard.h"
class SDMJFanxingXiaoMenQing
	:public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_XiaoMenQing; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		return ((SDMJPlayerCard*)pPlayerCard)->checkXiaoMenQing();
	}
};
