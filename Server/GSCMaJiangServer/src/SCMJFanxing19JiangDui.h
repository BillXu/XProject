#pragma once
#include "IFanxing.h"
#include "IMJPoker.h"
#include "SCMJPlayerCard.h"
class SCMJFanxing19JiangDui
	:public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_SC_19JiangDui; };

	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom) {
		return checkFanxing(pPlayerCard, pPlayer, nInvokerIdx, pmjRoom, false);
	}

	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom, bool ignorShun)
	{
		IMJPlayerCard::VEC_CARD vTemp;
		pPlayerCard->getPengedCard(vTemp);
		pPlayerCard->getMingGangedCard(vTemp);
		pPlayerCard->getAnGangedCard(vTemp);
		if (vTemp.size()) {
			if (std::find_if(vTemp.begin(), vTemp.end(), [](uint8_t &tCard) {
				uint8_t tValue = card_Value(tCard);
				return tValue != 1 && tValue != 9;
			}) != vTemp.end()) {
				return false;
			}
		}

		vTemp.clear();
		pPlayerCard->getHoldCard(vTemp);
		auto iter = std::find_if(vTemp.begin(), vTemp.end(), [](uint8_t &tCard) {
			auto tValue = card_Value(tCard);
			return tValue != 1 && tValue != 9;
		});
		while (iter != vTemp.end()) {
			if (ignorShun) {
				return false;
			}
			auto tCard = *iter;
			auto tValue = card_Value(tCard);
			if (tValue > 3 && tValue < 7) {
				return false;
			}
			auto tType = card_Type(tCard);
			if (tType > eCT_Tiao || tType < eCT_Wan) {
				return false;
			}
			uint8_t tCard_1, tCard_2, tCard_3;
			if (tValue < 4) {
				tCard_1 = make_Card_Num(tType, 1);
				tCard_2 = make_Card_Num(tType, 2);
				tCard_3 = make_Card_Num(tType, 3);
			}
			else {
				tCard_1 = make_Card_Num(tType, 9);
				tCard_2 = make_Card_Num(tType, 8);
				tCard_3 = make_Card_Num(tType, 7);
			}
			
			uint8_t tCount_1 = std::count(vTemp.begin(), vTemp.end(), tCard_1);
			uint8_t tCount_2 = std::count(vTemp.begin(), vTemp.end(), tCard_2);
			uint8_t tCount_3 = std::count(vTemp.begin(), vTemp.end(), tCard_3);
			if (tCount_1 < tCount_2 || tCount_2 != tCount_3) {
				return false;
			}
			for (uint8_t i = 0; i < tCount_2; i++) {
				eraseVector(tCard_1, vTemp);
				eraseVector(tCard_2, vTemp);
				eraseVector(tCard_3, vTemp);
			}

			iter = std::find_if(vTemp.begin(), vTemp.end(), [](uint8_t &tCard) {
				auto tValue = card_Value(tCard);
				return tValue != 1 && tValue != 9;
			});
		}

		if (((SCMJPlayerCard*)pPlayerCard)->isHoldCardCanHu(vTemp) == false) {
			return false;
		}
		return true;
	}

protected:
	void eraseVector(uint8_t p, IMJPlayerCard::VEC_CARD& typeVec) {
		auto it = find(typeVec.begin(), typeVec.end(), p);
		if (it != typeVec.end())
		{
			typeVec.erase(it);
		}
	}
};