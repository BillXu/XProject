#pragma once
#include "Fanxing7Dui.h"
class YZMJFanxing7Dui
	:public Fanxing7Dui
{
public:
	//此方法弃用
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override { return false; }
	
	//调用此方法获取真实7对胡的类型
	bool checkFanxing(IMJPlayerCard* pPlayerCard, eFanxingType& nType, uint8_t nInvokerIdx = -1);
};