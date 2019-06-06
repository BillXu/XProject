#include "SZMJFanxingHaiDiLaoYue.h"
#include "SZMJRoom.h"
bool SZMJFanxingHaiDiLaoYue::checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom) {
	if (pPlayer->getIdx() != nInvokerIdx) {
		return false;
	}
	//��������ͬʱ���㺣���ڣ��˷��������������д
	return pmjRoom->getPoker()->getLeftCardCount() < pmjRoom->getSeatCnt();
}