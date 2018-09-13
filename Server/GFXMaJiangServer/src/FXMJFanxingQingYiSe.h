#pragma once
#include "FanxingQingYiSe.h"
class FXMJFanxingQingYiSe
	:public FanxingQingYiSe
{
public:
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		IMJPlayerCard::VEC_CARD vCards;
		pPlayerCard->getHoldCard(vCards);
		auto pCheckType = [](IMJPlayerCard::VEC_CARD& vCard, uint8_t nType)
		{
			if (nType == eCT_Feng || nType == eCT_Jian) {
				return false;
			}

			for (auto& nCard : vCard)
			{
				auto tType = card_Type(nCard);
				if (tType != nType)
				{
					return false;
				}
			}
			return true;
		};

		// check hold card ;
		auto nType = card_Type(vCards.front());
		if (pCheckType(vCards, nType) == false)
		{
			return false;
		}

		// check peng 
		vCards.clear();
		pPlayerCard->getPengedCard(vCards);
		if (pCheckType(vCards, nType) == false)
		{
			return false;
		}
		// check direct gang ;
		vCards.clear();
		pPlayerCard->getMingGangedCard(vCards);
		if (pCheckType(vCards, nType) == false)
		{
			return false;
		}
		// check an gang ;
		vCards.clear();
		pPlayerCard->getAnGangedCard(vCards);
		if (pCheckType(vCards, nType) == false)
		{
			return false;
		}
		// check eat ;
		vCards.clear();
		pPlayerCard->getEatedCard(vCards);
		if (pCheckType(vCards, nType) == false)
		{
			return false;
		}
		return true;
	}
};