#pragma once
#include "IFanxing.h"
#include "IMJPlayer.h"
#include "IMJRoom.h"
#include "ARQMJPlayerCard.h"
class ARQMJFanxingJiaHu
	:public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_JiaHu; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		/*if (pPlayer->getIdx() != nInvokerIdx)
		{
			return false;
		}*/

		return ((ARQMJPlayerCard*)pPlayerCard)->isHuOnly19() || ((ARQMJPlayerCard*)pPlayerCard)->isDanDiao() || ((ARQMJPlayerCard*)pPlayerCard)->isJiaHu();
	}
};