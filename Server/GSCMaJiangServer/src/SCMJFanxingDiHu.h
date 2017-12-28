#pragma once
#include "IFanxing.h"
#include "IMJPlayer.h"
#include "IMJRoom.h"
class SCMJFanxingDiHu
	:public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_DiHu; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		if (pPlayer->getIdx() != nInvokerIdx)
		{
			return false;
		}

		if (pPlayer->getIdx() == pmjRoom->getBankerIdx()) {
			return false;
		}

		if (!pPlayer->haveFlag(IMJPlayer::eMJActFlag_WaitCheckTianTing))
		{
			return false;
		}
		return true;
	}
};