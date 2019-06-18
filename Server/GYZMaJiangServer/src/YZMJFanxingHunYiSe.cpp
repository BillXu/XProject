#include "YZMJFanxingHunYiSe.h"
#include "YZMJPlayerCard.h"
bool YZMJFanxingHunYiSe::checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom) {
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

	auto nType = eCT_None;
	bool bFoundFeng = false;
	for (auto nCard : vAllCard) {
		auto nType_ = card_Type(nCard);
		if (nType == eCT_None) {
			if (nType_ == eCT_Tiao || nType_ == eCT_Wan || nType_ == eCT_Tong)
			{
				nType = nType_;
				continue;
			}
		}
		if (!bFoundFeng) {
			if (nType_ == eCT_Feng || nType_ == eCT_Jian) {
				bFoundFeng = true;
				continue;
			}
		}
		if (nType_ == eCT_Tiao || nType_ == eCT_Wan || nType_ == eCT_Tong)
		{
			if (nType_ != nType) {
				return false;
			}
		}
	}
	return bFoundFeng;
}