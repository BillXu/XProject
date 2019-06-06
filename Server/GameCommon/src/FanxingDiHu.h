#pragma once
#include "IFanxing.h"
#include "IMJPlayer.h"
class FanxingDiHu
	:public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_DiHu; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		return pPlayer->haveFlag(IMJPlayer::eMJActFlag_WaitCheckTianTing);
	}
};