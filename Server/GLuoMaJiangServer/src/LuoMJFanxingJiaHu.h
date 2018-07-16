#pragma once
#include "IFanxing.h"
#include "IMJPlayer.h"
#include "IMJRoom.h"
#include "LuoMJPlayerCard.h"
class LuoMJFanxingJiaHu
	:public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_JiaHu; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		if (pPlayer->getIdx() != nInvokerIdx)
		{
			return false;
		}

		return ((LuoMJPlayerCard*)pPlayerCard)->isHuOnly19() || ((LuoMJPlayerCard*)pPlayerCard)->isDanDiao() || ((LuoMJPlayerCard*)pPlayerCard)->isJiaHu();
	}
};