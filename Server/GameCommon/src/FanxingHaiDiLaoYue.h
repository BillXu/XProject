#pragma once
#include "IFanxing.h"
#include "IMJRoom.h"
#include "IPoker.h"
#include "IMJPlayer.h"
class FanxingHaiDiLaoYue
	:public IFanxing
{
public:
	uint16_t getFanxingType()override { return eFanxing_HaiDiLaoYue; };
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		//海底捞月同时计算海底炮，此方法重用需谨慎复写
		return pmjRoom->getPoker()->getLeftCardCount() == 0;
	}
};