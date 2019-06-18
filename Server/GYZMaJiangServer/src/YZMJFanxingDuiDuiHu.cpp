#include "YZMJFanxingDuiDuiHu.h"
#include "YZMJPlayerCard.h"
bool YZMJFanxingDuiDuiHu::checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom) {
	IMJPlayerCard::VEC_CARD vAllCard;
	pPlayerCard->getHoldCard(vAllCard);
	uint8_t daCnt = ((YZMJPlayerCard*)pPlayerCard)->daFilter(vAllCard);

	if (vAllCard.size() < 1) {
		return true;
	}

	std::sort(vAllCard.begin(), vAllCard.end());
	bool bFindJiang = false;
	uint8_t needDaCnt(0);
	for (uint8_t i = 0; i < vAllCard.size(); i++) {
		if (i + 2 < vAllCard.size() && vAllCard[i] == vAllCard[i + 2]) {
			i += 2;
			continue;
		}
		else if (i + 1 < vAllCard.size()) {
			if (vAllCard[i] == vAllCard[i + 1]) {
				if (bFindJiang) {
					needDaCnt++;
				}
				else {
					bFindJiang = true;
				}
				i++;
			}
			else {
				if (bFindJiang) {
					needDaCnt += 2;
				}
				else {
					bFindJiang = true;
					needDaCnt++;
				}
			}
		}
		else {
			if (bFindJiang) {
				needDaCnt += 2;
			}
			else {
				bFindJiang = true;
				needDaCnt++;
			}
		}
		if (needDaCnt > daCnt) {
			return false;
		}
	}

	if (!bFindJiang) {
		if (daCnt - needDaCnt > 1) {
			bFindJiang = true;
		}
	}

	return bFindJiang;
}