#pragma once
#include "IFanxing.h"
#include "IMJPlayer.h"
class FanxingGangKai
	:public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_GangKai; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		if (pPlayer->getIdx() != nInvokerIdx)
		{
			return false;
		}

		if (!pPlayer->haveFlag(IMJPlayer::eMJActFlag_Gang))
		{
			return false;
		}
		return true;
	}
};