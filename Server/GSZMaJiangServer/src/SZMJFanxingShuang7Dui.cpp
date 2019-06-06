#include "SZMJFanxingShuang7Dui.h"
#include "SZMJRoom.h"
#include "log4z.h"
bool SZMJFanxingShuang7Dui::checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom) {
	if (pmjRoom == nullptr) {
		LOGFMTE("why check SZMJ shuang7dui room is null");
		return false;
	}
	if (((SZMJRoom*)pmjRoom)->isEanableHHQD()) {
		return FanxingShuang7Dui::checkFanxing(pPlayerCard, pPlayer, nInvokerIdx, pmjRoom);
	}
	return false;
}