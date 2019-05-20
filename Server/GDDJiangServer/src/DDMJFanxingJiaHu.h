#pragma once
#include "IFanxing.h"
#include "IMJPlayer.h"
#include "IMJRoom.h"
#include "DDMJPlayerCard.h"
class DDMJFanxingJiaHu
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

		return ((DDMJPlayerCard*)pPlayerCard)->isHuOnly19() || ((DDMJPlayerCard*)pPlayerCard)->isDanDiao() || ((DDMJPlayerCard*)pPlayerCard)->isJiaHu();
	}
};