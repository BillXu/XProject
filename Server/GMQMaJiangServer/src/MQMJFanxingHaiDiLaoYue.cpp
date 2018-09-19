#include "MQMJFanxingHaiDiLaoYue.h"
#include "MQMJRoom.h"
bool MQMJFanxingHaiDiLaoYue::checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom) {
	if (pPlayer->getIdx() != nInvokerIdx) {
		return false;
	}
	//��������ͬʱ���㺣���ڣ��˷��������������д
	auto pRoom = (MQMJRoom*)pmjRoom;
	return pRoom->needChu() == false;
}