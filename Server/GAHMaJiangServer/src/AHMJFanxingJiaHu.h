#pragma once
#include "IFanxing.h"
#include "IMJPlayer.h"
#include "IMJRoom.h"
#include "AHMJPlayerCard.h"
class AHMJFanxingJiaHu
	:public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_JiaHu; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		return ((AHMJPlayerCard*)pPlayerCard)->isJiaHu() && ((AHMJPlayerCard*)pPlayerCard)->canHuOnlyOneCard();
	}
};