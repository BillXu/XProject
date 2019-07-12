#include "NJMJFanxingDiHu.h"
#include "NJMJPlayerCard.h"
bool NJMJFanxingDiHu::checkFanxing(IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom) {
	auto pCard = (NJMJPlayerCard*)pPlayer->getPlayerCard();
	return pCard->isTing();
}