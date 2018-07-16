#pragma once
#include "IFanxing.h"
#include "IMJPlayer.h"
#include "IMJRoom.h"
class LuoMJFanxingGangHouPao
	:public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_GangHouPao; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		if (pPlayer->getIdx() != nInvokerIdx)
		{
			return false;
		}

		auto pInvoker = (IMJPlayer*)pmjRoom->getPlayerByIdx(nInvokerIdx);
		if (pInvoker) {
			if (pInvoker->haveFlag(IMJPlayer::eMJActFlag_Gang) == false) {
				return false;
			}
		}
		else {
			return false;
		}
		return true;
	}
};