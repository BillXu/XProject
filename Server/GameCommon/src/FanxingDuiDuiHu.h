#pragma once
#include "IFanxing.h"
#include <algorithm>
#include "log4z.h"
class FanxingDuiDuiHu
	:public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_DuiDuiHu; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		IMJPlayerCard::VEC_CARD vCards;
		pPlayerCard->getEatedCard(vCards);
		if (vCards.size()) {
			return false;
		}

		vCards.clear();
		pPlayerCard->getHoldCard(vCards);
		if (vCards.size() % 3 != 2)
		{
			LOGFMTE( "you are not hu , ok ? do not check dudui hu " );
			return false;
		}

		std::sort(vCards.begin(),vCards.end());
		bool isFindJiang = false;
		for (uint8_t nIdx = 0; ( nIdx + 1 ) < vCards.size(); )
		{
			if ((nIdx + 2) < vCards.size() && vCards[nIdx] == vCards[nIdx + 2])
			{
				nIdx += 3;
				continue;
			}

			if ( vCards[nIdx] == vCards[nIdx + 1] && isFindJiang == false)
			{
				isFindJiang = true;
				nIdx += 2;
				continue;
			}

			return false;
		}
		return true;
	}
};