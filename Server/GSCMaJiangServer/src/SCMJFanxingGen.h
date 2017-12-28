#pragma once
#include "IFanxing.h"
#include <algorithm>
class SCMJFanxingGen
	:public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_SC_Gen; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		std::vector<eFanxingType> vFanxing;
		return checkFanxing(pPlayerCard, vFanxing);
	}

	uint8_t checkFanxing(IMJPlayerCard* pPlayerCard, std::vector<eFanxingType>& vFanxing) {
		uint8_t nGenCnt = 0;
		IMJPlayerCard::VEC_CARD vTemp;
		pPlayerCard->getMingGangedCard(vTemp);
		pPlayerCard->getAnGangedCard(vTemp);
		for (uint8_t i = 0; i < vTemp.size(); i++) {
			vFanxing.push_back(eFanxing_SC_Gen);
			nGenCnt++;
		}
		vTemp.clear();
		pPlayerCard->getPengedCard(vTemp);
		pPlayerCard->getPengedCard(vTemp);
		pPlayerCard->getPengedCard(vTemp);
		pPlayerCard->getHoldCard(vTemp);
		std::sort(vTemp.begin(), vTemp.end());
		for (uint8_t i = 0; i < vTemp.size(); i++) {
			if (i + 1 < vTemp.size() && vTemp[i] == vTemp[i + 1]) {
				continue;
			}
			uint8_t tCnt = std::count(vTemp.begin(), vTemp.end(), vTemp[i]);
			if (tCnt == 4) {
				vFanxing.push_back(eFanxing_SC_Gen);
				nGenCnt++;
			}
		}
		return nGenCnt;
	}
};