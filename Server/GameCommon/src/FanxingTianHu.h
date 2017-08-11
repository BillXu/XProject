#pragma once
#include "IFanxing.h"
#include "IMJPlayer.h"
class FanxingTianHu
	:public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_TianHu; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		if (pPlayer->getIdx() != nInvokerIdx)
		{
			return false;
		}

		if ( !pPlayer->haveFlag(IMJPlayer::eMJActFlag_CanTianHu))
		{
			return false;
		}
		return true;
	}
};
