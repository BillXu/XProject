#pragma once
#include "IFanxing.h"
#include "IMJPoker.h"
class FanxingHunYiSe
	:public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_HunYiSe; }

	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		IMJPlayerCard::VEC_CARD vCards;
		pPlayerCard->getHoldCard(vCards);
		pPlayerCard->getEatedCard(vCards);
		pPlayerCard->getPengedCard(vCards);
		pPlayerCard->getMingGangedCard(vCards);
		pPlayerCard->getAnGangedCard(vCards);

		bool bFindFeng = false;
		do
		{
			auto iter = std::find_if(vCards.begin(), vCards.end(), [](uint8_t n) { return card_Type(n) == eCT_Feng; });
			if (iter != vCards.end())
			{
				bFindFeng = true;
				vCards.erase(iter);
			}
			else
			{
				break;
			}
		} while (true);

		if (bFindFeng == false || vCards.empty())
		{
			return false;
		}

		auto nType = card_Type(vCards.front());
		for (auto& ref : vCards)
		{
			auto tt = card_Type(ref);
			if (nType != tt)
			{
				return false;
			}
		}

		return true;
	}
};