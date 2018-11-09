#pragma once
#include "IFanxing.h"
#include "IMJPoker.h"
class CFMJFanxing13Yao
	: public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_13Yao; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		IMJPlayerCard::VEC_CARD vCards;
		pPlayerCard->getHoldCard(vCards);
		if (vCards.size() != 14)
		{
			return false;
		}
		bool bFindJiang = false;
		for (uint8_t nIdx = 0; (nIdx + 1) < vCards.size(); nIdx++) {
			auto eType = card_Type(vCards[nIdx]);
			uint8_t nextIdx = nIdx + 1;
			if (nextIdx < vCards.size() && vCards[nIdx] == vCards[nextIdx]) {
				uint8_t nextIdx_1 = nextIdx + 1;
				if (nextIdx_1 < vCards.size() && vCards[nIdx] == vCards[nextIdx_1]) {
					return false;
				}
				if (bFindJiang) {
					return false;
				}
				else {
					bFindJiang = true;
					nIdx++;
				}
			}
			if (eCT_Feng == eType || eCT_Jian == eType) {
				continue;
			}
			if (eCT_Tiao == eType || eCT_Tong == eType || eCT_Wan == eType) {
				auto eValue = card_Value(vCards[nIdx]);
				if (1 == eValue || 9 == eValue) {
					continue;
				}
			}
			return false;
		}
		return true;
	}
};