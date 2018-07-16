#pragma once
#include "FanxingHaiDiLaoYue.h"
class MQMJFanxingHaiDiLaoYue
	:public FanxingHaiDiLaoYue
{
public:
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		//海底捞月同时计算海底炮，此方法重用需谨慎复写
		return pmjRoom->getPoker()->getLeftCardCount() < 20;
	}
};