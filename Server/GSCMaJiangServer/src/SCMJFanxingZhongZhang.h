#pragma once
#include "IFanxing.h"
#include "IMJPoker.h"
class SCMJFanxingZhongZhang
	:public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_SC_ZhongZhang; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		IMJPlayerCard::VEC_CARD vTemp;
		pPlayerCard->getPengedCard(vTemp);
		pPlayerCard->getMingGangedCard(vTemp);
		pPlayerCard->getEatedCard(vTemp);
		pPlayerCard->getAnGangedCard(vTemp);
		pPlayerCard->getHoldCard(vTemp);
		if (vTemp.size()) {
			return std::find_if(vTemp.begin(), vTemp.end(), [](uint8_t &tCard) {
				uint8_t tValue = card_Value(tCard);
				return tValue == 1 || tValue == 9;
			}) == vTemp.end();
		}
		return false;
	}
};