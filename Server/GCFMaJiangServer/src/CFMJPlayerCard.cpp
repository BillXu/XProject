#include "CFMJPlayerCard.h"
#include "CFMJFanxingChecker.h"
CFMJFanxingChecker CFMJPlayerCard::m_pFanxingChecker = CFMJFanxingChecker();
CFMJPlayerCard::CFMJPlayerCard() {
	//test_FanxingChecker.get()->init();
	//m_pFanxingChecker = new CFMJFanxingChecker();
	//m_pFanxingChecker->init();
}

CFMJPlayerCard::~CFMJPlayerCard() {
	//delete m_pFanxingChecker;
	//m_pFanxingChecker = nullptr;
}

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

uint8_t CFMJPlayerCard::getLastHuCnt() {
	if (m_nLastHuCnt) {
		return m_nLastHuCnt - 1;
	}
	return 0;
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
		m_nLastHuCnt = 1;
		getFanXingAndFanCnt(m_vLastHuFanxing, m_nLastHuCnt);
	}
	return bCanHu;
}

bool CFMJPlayerCard::check13Yao() {
	CFMJFanxing13Yao cFanxing13Yao;
	return cFanxing13Yao.checkFanxing(this, nullptr, 0, nullptr);
}

void CFMJPlayerCard::getFanXingAndFanCnt(std::vector<eFanxingType>& vHuTypes, uint8_t& nFanCnt) {
	m_pFanxingChecker.checkFanxing(vHuTypes, this, 0, nullptr);
	sortFanCnt(vHuTypes, nFanCnt);
}

void CFMJPlayerCard::sortFanCnt(std::vector<eFanxingType>& vHuTypes, uint8_t& nFanCnt) {
	for (auto& ref : vHuTypes) {
		switch (ref)
		{
		case eFanxing_QiDui:
		{
			nFanCnt += 2;
		}
		break;
		case eFanxing_ShuangQiDui:
		{
			nFanCnt += 3;
		}
		break;
		case eFanxing_DuiDuiHu:
		{
			nFanCnt += 1;
		}
		break;
		case eFanxing_13Yao:
		{
			nFanCnt += 4;
		}
		break;
		}
	}
}