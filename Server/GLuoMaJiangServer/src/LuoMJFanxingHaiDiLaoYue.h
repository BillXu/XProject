#pragma once
#include "FanxingHaiDiLaoYue.h"
class LuoMJFanxingHaiDiLaoYue
	:public FanxingHaiDiLaoYue
{
public:
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		//��������ͬʱ���㺣���ڣ��˷��������������д
		return false;
		//return pmjRoom->getPoker()->getLeftCardCount() < 12;
	}
};