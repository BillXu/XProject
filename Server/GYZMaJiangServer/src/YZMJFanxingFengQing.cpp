#include "YZMJFanxingFengQing.h"
#include "YZMJPlayerCard.h"
#include "IMJPoker.h"
bool YZMJFanxingFengQing::checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom) {
	IMJPlayerCard::VEC_CARD vAllCard;
	pPlayerCard->getHoldCard(vAllCard);

	//¹ýÂËµôËùÓÐ´î
	((YZMJPlayerCard*)pPlayerCard)->daFilter(vAllCard);

	IMJPlayerCard::VEC_CARD vTemp;
	pPlayerCard->getAnGangedCard(vTemp);
	vAllCard.insert(vAllCard.end(), vTemp.begin(), vTemp.end());

	vTemp.clear();
	pPlayerCard->getMingGangedCard(vTemp);
	vAllCard.insert(vAllCard.end(), vTemp.begin(), vTemp.end());

	vTemp.clear();
	pPlayerCard->getPengedCard(vTemp);
	vAllCard.insert(vAllCard.end(), vTemp.begin(), vTemp.end());

	for (auto& ref : vAllCard)
	{
		auto tt = card_Type(ref);
		if (tt != eCT_Feng && tt != eCT_Jian)
		{
			return false;
		}
	}
	return true;
}