#pragma once
#include "FanxingMenQing.h"
#include "FXMJPlayerCard.h"
class FXMJFanxingMenQing
	:public FanxingMenQing
{
public:
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		if (((FXMJPlayerCard*)pPlayerCard)->isPreGang()) {
			return false;
		}

		IMJPlayerCard::VEC_CARD vTemp;
		pPlayerCard->getPengedCard(vTemp);
		pPlayerCard->getMingGangedCard(vTemp);
		pPlayerCard->getEatedCard(vTemp);
		if (vTemp.empty()) {
			if (((FXMJPlayerCard*)pPlayerCard)->isEanble7Pair()) {
				IMJPlayerCard::VEC_CARD vCards;
				pPlayerCard->getHoldCard(vCards);
				uint8_t nPairCnt = 0;
				for (uint8_t i = 0; i < vCards.size(); i++) {
					if (i + 1 < vCards.size() && vCards[i] == vCards[i + 1]) {
						nPairCnt++;
						if (nPairCnt > 4) {
							return false;
						}
						i++;
					}
				}
			}
			return true;
		}
		return false;
	}
};