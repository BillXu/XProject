#include "YZMJFanxingQingYiSe.h"
#include "YZMJPlayerCard.h"
bool YZMJFanxingQingYiSe::checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom) {
	IMJPlayerCard::VEC_CARD vAllCard, vTemp;
	pPlayerCard->getHoldCard(vAllCard);

	((YZMJPlayerCard*)pPlayerCard)->daFilter(vAllCard);

	pPlayerCard->getAnGangedCard(vTemp);
	vAllCard.insert(vAllCard.end(), vTemp.begin(), vTemp.end());

	vTemp.clear();
	pPlayerCard->getMingGangedCard(vTemp);
	vAllCard.insert(vAllCard.end(), vTemp.begin(), vTemp.end());

	vTemp.clear();
	pPlayerCard->getPengedCard(vTemp);
	vAllCard.insert(vAllCard.end(), vTemp.begin(), vTemp.end());

	auto nType = card_Type(vAllCard.at(0));
	if (nType != eCT_None)
	{
		for (auto& ref : vAllCard)
		{
			auto tt = card_Type(ref);
			if (nType != tt)
			{
				return false;
			}
		}
	}
	else
	{
		return false;
	}

	return true;
}