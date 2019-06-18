#include "YZMJFanxingYiTiaoLong.h"
#include "YZMJPlayerCard.h"
bool YZMJFanxingYiTiaoLong::checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom) {
	return ((YZMJPlayerCard*)pPlayerCard)->checkYiTiaoLong();
}