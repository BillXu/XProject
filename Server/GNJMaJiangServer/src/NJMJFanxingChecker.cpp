#include "NJMJFanxingChecker.h"
#include "NJMJPlayerCard.h"
#include "NJMJRoom.h"
void NJMJFanxingChecker::checkFanxing(IMJPlayer* pPlayer, uint8_t nInvokerIdx, NJMJRoom::stSortFanInformation& stInformation) {
	auto pCard = (NJMJPlayerCard*)pPlayer->getPlayerCard();
	bool bZiMo = nInvokerIdx == pPlayer->getIdx();
	bool bKuaiZhaoHu = false;

	if (bZiMo) {
		bKuaiZhaoHu = pCard->checkKuaiZhaoZiMo(stInformation.m_nBaoIdx, stInformation.m_vFanxing);
	}
	else {
		bKuaiZhaoHu = pCard->checkKuaiZhaoHu(stInformation.m_nBaoIdx, stInformation.m_vFanxing);
	}

	if (bKuaiZhaoHu) {
		stInformation.m_bWaiBao = true;
	}
	else {
		//TODO
		ss
	}

	stInformation.m_nHoldHuaCnt = pCard->getHuaCntWithoutHuTypeHuaCnt(stInformation.m_vFanxing, bKuaiZhaoHu);

	sortHuHuaCnt(stInformation.m_nHuHuaCnt, stInformation.m_vFanxing);
}