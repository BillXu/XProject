#pragma once
#include "IFanxing.h"
#include <algorithm>
class Fanxing7Dui
	:public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_QiDui; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		IMJPlayerCard::VEC_CARD vCards;
		pPlayerCard->getHoldCard(vCards);
		if ( vCards.size() != 14 )
		{
			return false;
		}
		std::sort(vCards.begin(), vCards.end());
	
		for ( uint8_t nIdx = 0; (nIdx + 1) < vCards.size(); nIdx += 2 )
		{
			if (vCards[nIdx] != vCards[nIdx + 1])
			{
				return false;
			}
		}
		return true;
	}
};
