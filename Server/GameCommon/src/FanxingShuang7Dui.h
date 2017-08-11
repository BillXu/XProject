#pragma once
#include "Fanxing7Dui.h"
class FanxingShuang7Dui
	:public Fanxing7Dui
{
public:
	uint16_t getFanxingType()override { return eFanxing_ShuangQiDui; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		if (!Fanxing7Dui::checkFanxing(pPlayerCard, pPlayer, nInvokerIdx, pmjRoom))
		{
			return false;
		}

		IMJPlayerCard::VEC_CARD vCards;
		pPlayerCard->getHoldCard(vCards);

		std::sort(vCards.begin(), vCards.end());

		for (uint8_t nIdx = 0; (nIdx + 3) < vCards.size(); nIdx += 2 )
		{
			if (vCards[nIdx] == vCards[nIdx + 3])
			{
				return true;
			}
		}
		return false;
	}
};
