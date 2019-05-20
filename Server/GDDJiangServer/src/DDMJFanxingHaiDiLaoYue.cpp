#include "DDMJFanxingHaiDiLaoYue.h"
#include "DDMJRoom.h"
bool DDMJFanxingHaiDiLaoYue::checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom) {
	if (pPlayer->getIdx() != nInvokerIdx) {
		return false;
	}
	//海底捞月同时计算海底炮，此方法重用需谨慎复写
	auto pRoom = (DDMJRoom*)pmjRoom;
	return pRoom->needChu() == false;
}