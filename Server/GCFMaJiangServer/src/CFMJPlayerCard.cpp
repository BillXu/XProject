#include "CFMJPlayerCard.h"
#include "CFMJFanxingChecker.h"
void CFMJPlayerCard::reset() {
	MJPlayerCard::reset();
	clearHuCnt();
}

void CFMJPlayerCard::onLouHu() {
	if (m_nLastHuCnt > m_nLouHuCnt) {
		m_nLouHuCnt = m_nLastHuCnt;
	}
}

void CFMJPlayerCard::clearHuCnt() {
	m_nLastHuCnt = 0;
	m_nLouHuCnt = 0;
	m_vLastHuFanxing.clear();
}

void CFMJPlayerCard::getLastHuFanxing(std::vector<eFanxingType>& vFanxing) {
	vFanxing.insert(vFanxing.end(), m_vLastHuFanxing.begin(), m_vLastHuFanxing.end());
}

bool CFMJPlayerCard::canHuWitCard(uint8_t nCard) {
	bool bCanHu = MJPlayerCard::canHuWitCard(nCard);
	if (bCanHu) {
		return m_nLastHuCnt > m_nLouHuCnt;
	}
}

bool CFMJPlayerCard::isHoldCardCanHu(uint8_t& nJiang) {
	bool bCanHu = check13Yao() || MJPlayerCard::isHoldCardCanHu(nJiang);
	if (bCanHu) {
		m_vLastHuFanxing.clear();
		m_nLastHuCnt = 0;
		getFanXingAndFanCnt(m_vLastHuFanxing, m_nLastHuCnt);
	}
	return bCanHu;
}