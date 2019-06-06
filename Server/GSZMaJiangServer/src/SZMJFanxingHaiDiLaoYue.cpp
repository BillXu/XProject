#include "SZMJFanxingHaiDiLaoYue.h"
#include "SZMJRoom.h"
bool SZMJFanxingHaiDiLaoYue::checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom) {
	if (pPlayer->getIdx() != nInvokerIdx) {
		return false;
	}
	//海底捞月同时计算海底炮，此方法重用需谨慎复写
	return pmjRoom->getPoker()->getLeftCardCount() < pmjRoom->getSeatCnt();
}