#include "NCMJPlayerCard.h"
#include "NCMJFanxingChecker.h"
NCMJFanxingChecker NCMJPlayerCard::m_pFanxingChecker = NCMJFanxingChecker();
NCMJPlayerCard::NCMJPlayerCard() {
	//test_FanxingChecker.get()->init();
	//m_pFanxingChecker = new NCMJFanxingChecker();
	//m_pFanxingChecker->init();
}

NCMJPlayerCard::~NCMJPlayerCard() {
	//delete m_pFanxingChecker;
	//m_pFanxingChecker = nullptr;
}

void NCMJPlayerCard::reset() {
	MJPlayerCard::reset();
	clearHuCnt();
}

void NCMJPlayerCard::onLouHu() {
	if (m_nLastHuCnt > m_nLouHuCnt) {
		m_nLouHuCnt = m_nLastHuCnt;
	}
}

void NCMJPlayerCard::clearHuCnt() {
	m_nLastHuCnt = 0;
	m_nLouHuCnt = 0;
	m_vLastHuFanxing.clear();
}

uint8_t NCMJPlayerCard::getLastHuCnt() {
	if (m_nLastHuCnt) {
		return m_nLastHuCnt - 1;
	}
	return 0;
}

void NCMJPlayerCard::getLastHuFanxing(std::vector<eFanxingType>& vFanxing) {
	vFanxing.insert(vFanxing.end(), m_vLastHuFanxing.begin(), m_vLastHuFanxing.end());
}

bool NCMJPlayerCard::canHuWitCard(uint8_t nCard) {
	bool bCanHu = MJPlayerCard::canHuWitCard(nCard);
	if (bCanHu) {
		return m_nLastHuCnt > m_nLouHuCnt;
	}
}

bool NCMJPlayerCard::isHoldCardCanHu(uint8_t& nJiang) {
	bool bCanHu = MJPlayerCard::isHoldCardCanHu(nJiang);
	if (bCanHu) {
		m_vLastHuFanxing.clear();
		m_nLastHuCnt = 1;
		getFanXingAndFanCnt(m_vLastHuFanxing, m_nLastHuCnt);
	}
	return bCanHu;
}

void NCMJPlayerCard::getFanXingAndFanCnt(std::vector<eFanxingType>& vHuTypes, uint8_t& nFanCnt) {
	m_pFanxingChecker.checkFanxing(vHuTypes, this, 0, nullptr);
	sortFanCnt(vHuTypes, nFanCnt);
}

void NCMJPlayerCard::sortFanCnt(std::vector<eFanxingType>& vHuTypes, uint8_t& nFanCnt) {
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
		}
	}
}