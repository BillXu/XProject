#pragma once
#include "Fanxing7Dui.h"
class YZMJFanxing7Dui
	:public Fanxing7Dui
{
public:
	//�˷�������
	bool checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override { return false; }
	
	//���ô˷�����ȡ��ʵ7�Ժ�������
	bool checkFanxing(IMJPlayerCard* pPlayerCard, eFanxingType& nType, uint8_t nInvokerIdx = -1);
};