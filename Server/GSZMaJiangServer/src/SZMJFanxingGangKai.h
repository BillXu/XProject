#pragma once
#include "FanxingGangKai.h"
class SZMJFanxingGangKai
	:public FanxingGangKai
{
public:
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		if (pPlayer->getIdx() != nInvokerIdx)
		{
			return false;
		}

		return pPlayer->haveFlag(IMJPlayer::eMJActFlag_Gang) || pPlayer->haveFlag(IMJPlayer::eMJActFlag_BuHua);
	}
};