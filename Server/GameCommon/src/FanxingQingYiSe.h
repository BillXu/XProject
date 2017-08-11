#pragma once
#include "IFanxing.h"
#include "log4z.h"
#include "MJCard.h"
class FanxingQingYiSe
	:public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_QingYiSe; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		IMJPlayerCard::VEC_CARD vCards;
		pPlayerCard->getHoldCard(vCards);
		auto pCheckType = [](IMJPlayerCard::VEC_CARD& vCard, uint8_t nType)
		{
			for (auto& nCard : vCard)
			{
				if (card_Type(nCard) != nType)
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