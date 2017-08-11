#pragma once
#include "IFanxing.h"
class FanxingQuanQiuDuDiao
	:public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_QuanQiuDuDiao; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		IMJPlayerCard::VEC_CARD vCards;
		pPlayerCard->getHoldCard(vCards);
		return vCards.size() == 2;
	}
};