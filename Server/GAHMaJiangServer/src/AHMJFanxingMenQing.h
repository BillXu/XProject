#pragma once
#include "FanxingMenQing.h"
class AHMJFanxingMenQing
	: public FanxingMenQing
{
public:
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		IMJPlayerCard::VEC_CARD vTemp;
		pPlayerCard->getPengedCard(vTemp);
		pPlayerCard->getMingGangedCard(vTemp);
		pPlayerCard->getEatedCard(vTemp);
		pPlayerCard->getAnGangedCard(vTemp);
		return vTemp.empty();
	}
};