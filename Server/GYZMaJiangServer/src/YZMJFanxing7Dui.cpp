#include "YZMJFanxing7Dui.h"
#include "YZMJPlayerCard.h"
bool YZMJFanxing7Dui::checkFanxing(IMJPlayerCard* pPlayerCard, eFanxingType& nType, uint8_t nInvokerIdx) {
	if (((YZMJPlayerCard*)pPlayerCard)->isEanble7Pair() == false) {
		return false;
	}

	IMJPlayerCard::VEC_CARD vHoldCard;
	pPlayerCard->getHoldCard(vHoldCard);
	uint8_t nDaCnt = 0;
	if (((YZMJPlayerCard*)pPlayerCard)->vecHu7Pair(vHoldCard, nDaCnt)) {
		uint8_t f4Cnt = 0;
		for (uint8_t i(0); i < vHoldCard.size(); i++) {
			if (i + 3 < vHoldCard.size()) {
				if (vHoldCard[i] == vHoldCard[i + 3]) {
					f4Cnt++;
					i += 3;
					continue;
				}
			}
			if (i + 2 < vHoldCard.size()) {
				if (vHoldCard[i] == vHoldCard[i + 2]) {
					f4Cnt++;
					i += 2;
					continue;
				}
			}
			if (i + 1 < vHoldCard.size()) {
				if (nDaCnt > 1 && vHoldCard[i] == vHoldCard[i + 1]) {
					f4Cnt++;
					nDaCnt -= 2;
					i += 1;
					continue;
				}
			}
		}

		switch (f4Cnt) {
		case 0: {
			nType = eFanxing_QiDui;
			break;
		}
		case 1: {
			nType = eFanxing_ShuangQiDui;
			break;
		}
		case 2: {
			nType = eFanxing_DoubleShuangQiDui;
			break;
		}
		case 3: {
			nType = eFanxing_TribleShuangQiDui;
			break;
		}
		default:
		{
			nType = eFanxing_QiDui;
			break;
		}
		}

		return true;
	}
	return false;
}