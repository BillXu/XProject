#include "ARQMJFanxingHaiDiLaoYue.h"
#include "ARQMJRoom.h"
bool ARQMJFanxingHaiDiLaoYue::checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom) {
	if (pPlayer->getIdx() != nInvokerIdx) {
		return false;
	}
	//��������ͬʱ���㺣���ڣ��˷��������������д
	auto pRoom = (ARQMJRoom*)pmjRoom;
	return pRoom->needChu() == false;
}