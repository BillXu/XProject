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
		if (pCard->getWaiBaoIdx(stInformation.m_nBaoIdx, bZiMo)) {
			stInformation.m_bWaiBao = true;
		}
		else if (pCard->getNormalBaoIdx(stInformation.m_nBaoIdx, bZiMo)) {
			stInformation.m_bBaoPai = true;
		}

		pCard->getNormalHuType(stInformation.m_vFanxing);
		/*auto pRoom = (NJMJRoom*)pPlayer->getRoom();

		if (checkFanxing(eFanxing_TianHu, pPlayer, nInvokerIdx, pRoom)) {
			stInformation.m_vFanxing.push_back(eFanxing_TianHu);
		}
		else if (checkFanxing(eFanxing_DiHu, pPlayer, nInvokerIdx, pRoom)) {
			stInformation.m_vFanxing.push_back(eFanxing_DiHu);
		}
		
		if (pCard->checkQiDui(stInformation.m_vFanxing) == false) {
			if (checkFanxing(eFanxing_DuiDuiHu, pPlayer, nInvokerIdx, pRoom)) {
				stInformation.m_vFanxing.push_back(eFanxing_DuiDuiHu);
			}

			if (checkFanxing(eFanxing_QuanQiuDuDiao, pPlayer, nInvokerIdx, pRoom)) {
				stInformation.m_vFanxing.push_back(eFanxing_QuanQiuDuDiao);
			}
		}*/
	}

	stInformation.m_nHoldHuaCnt = pCard->getHuaCntWithoutHuTypeHuaCnt(stInformation.m_vFanxing, bKuaiZhaoHu);

	sortHuHuaCnt(stInformation.m_nHuHuaCnt, stInformation.m_vFanxing);
}