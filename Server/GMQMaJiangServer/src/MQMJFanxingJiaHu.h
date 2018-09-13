#pragma once
#include "IFanxing.h"
#include "IMJPlayer.h"
#include "IMJRoom.h"
#include "MQMJPlayerCard.h"
class MQMJFanxingJiaHu
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

		return ((MQMJPlayerCard*)pPlayerCard)->isHuOnly19() || ((MQMJPlayerCard*)pPlayerCard)->isDanDiao() || ((MQMJPlayerCard*)pPlayerCard)->isJiaHu();
	}
};