#pragma once
#include "IFanxing.h"
#include "IMJPoker.h"
class FanxingMenQing
	:public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_MengQing; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		IMJPlayerCard::VEC_CARD vTemp;
		pPlayerCard->getPengedCard(vTemp);
		pPlayerCard->getMingGangedCard(vTemp);
		pPlayerCard->getEatedCard(vTemp);
		return vTemp.empty();
	}
};