#include "DDMJFanxingHaiDiLaoYue.h"
#include "DDMJRoom.h"
bool DDMJFanxingHaiDiLaoYue::checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom) {
	if (pPlayer->getIdx() != nInvokerIdx) {
		return false;
	}
	//��������ͬʱ���㺣���ڣ��˷��������������д
	auto pRoom = (DDMJRoom*)pmjRoom;
	return pRoom->needChu() == false;
}