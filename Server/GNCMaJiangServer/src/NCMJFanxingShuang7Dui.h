#pragma once
#include "IFanxing.h"
#include <algorithm>
class NCMJFanxingShuang7Dui
	: public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_ShuangQiDui; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		IMJPlayerCard::VEC_CARD vCards;
		pPlayerCard->getHoldCard(vCards);
		if (vCards.size() != 14)
		{
			return false;
		}
		std::sort(vCards.begin(), vCards.end());

		for (uint8_t nIdx = 0; (nIdx + 3) < vCards.size(); nIdx += 2)
		{
			if (vCards[nIdx] == vCards[nIdx + 3])
			{
				return true;
			}
		}
		return false;
	}
};