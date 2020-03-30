#pragma once
#include "IFanxing.h"
#include "IMJPlayer.h"
#include "IMJRoom.h"
class ARQMJFanxingBianHu
	:public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_BianHu; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		/*if (pPlayer->getIdx() != nInvokerIdx)
		{
			return false;
		}*/

		return ((ARQMJPlayerCard*)pPlayerCard)->isBianHu();
	}
};