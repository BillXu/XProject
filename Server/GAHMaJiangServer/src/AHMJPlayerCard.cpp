#include "AHMJPlayerCard.h"
#include "AHMJFanxingChecker.h"
#include "IMJPoker.h"
AHMJFanxingChecker AHMJPlayerCard::m_pFanxingChecker = AHMJFanxingChecker();

void AHMJPlayerCard::reset() {
	MJPlayerCard::reset();
	clearHuCnt();
	m_nHuCard = 0;
}

bool AHMJPlayerCard::canEatCard(uint8_t nCard, uint8_t nWithA, uint8_t nWithB) {
	VEC_CARD vTemp;
	getPengedCard(vTemp);
	if (std::find_if(vTemp.begin(), vTemp.end(), [](uint8_t& ref) {
		return card_Type(ref) == eCT_Jian;
	}) != vTemp.end()) {
		return false;
	}

	if (MJPlayerCard::canEatCard(nCard, nWithA, nWithB)) {
		VEC_CARD vHoldCards;
		getHoldCard(vHoldCards);
		if (vHoldCards.size() < 5) {
			VEC_CARD vTemp;
			getAnGangedCard(vTemp);
			if (vTemp.size() < 1) {
				return false;
			}
		}
		return true;
	}
	return false;
}

bool AHMJPlayerCard::canPengWithCard(uint8_t nCard) {
	auto nType = card_Type(nCard);
	if (nType == eCT_Jian) {
		VEC_CARD vTemp;
		getEatedCard(vTemp);
		if (vTemp.size()) {
			return false;
		}
	}

	if (MJPlayerCard::canPengWithCard(nCard)) {
		VEC_CARD vHoldCards;
		getHoldCard(vHoldCards);
		if (vHoldCards.size() < 5) {
			VEC_CARD vTemp;
			getEatedCard(vTemp);
			if (vTemp.size() > 0) {
				vTemp.clear();
				getAnGangedCard(vTemp);
				if (vTemp.size() < 1) {
					return false;
				}
			}
		}
		return true;
	}
	return false;
}

bool AHMJPlayerCard::canMingGangWithCard(uint8_t nCard) {
	if (MJPlayerCard::canMingGangWithCard(nCard)) {
		VEC_CARD vHoldCards;
		getHoldCard(vHoldCards);
		if (vHoldCards.size() < 5) {
			VEC_CARD vTemp;
			getEatedCard(vTemp);
			if (vTemp.size() > 0) {
				vTemp.clear();
				getAnGangedCard(vTemp);
				if (vTemp.size() < 1) {
					return false;
				}
			}
		}
		return true;
	}
	return false;
}

bool AHMJPlayerCard::canAnGangWithCard(uint8_t nCard) {
	if (checkMenQing()) {
		return false;
	}
	return MJPlayerCard::canAnGangWithCard(nCard);
}

bool AHMJPlayerCard::getHoldCardThatCanAnGang(VEC_CARD& vGangCards) {
	if (checkMenQing()) {
		return false;
	}
	return MJPlayerCard::getHoldCardThatCanAnGang(vGangCards);
}

void AHMJPlayerCard::onLouHu() {
	if (m_nLastHuCnt > m_nLouHuCnt) {
		m_nLouHuCnt = m_nLastHuCnt;
	}
}

void AHMJPlayerCard::clearHuCnt() {
	m_nLastHuCnt = 0;
	m_nLouHuCnt = 0;
	m_vLastHuFanxing.clear();
}

uint8_t AHMJPlayerCard::getLastHuCnt() {
	if (m_nLastHuCnt) {
		return m_nLastHuCnt - 1;
	}
	return 0;
}

void AHMJPlayerCard::getLastHuFanxing(std::vector<eFanxingType>& vFanxing) {
	vFanxing.insert(vFanxing.end(), m_vLastHuFanxing.begin(), m_vLastHuFanxing.end());
}

bool AHMJPlayerCard::canHuWitCard(uint8_t nCard) {
	setHuCard(nCard);
	bool bCanHu = MJPlayerCard::canHuWitCard(nCard);
	if (bCanHu) {
		return m_nLastHuCnt > m_nLouHuCnt;
	}
}

bool AHMJPlayerCard::isHoldCardCanHu(uint8_t& nJiang) {
	if (m_nHuCard == 0) {
		setHuCard(getNewestFetchedCard());
	}
	bool bCanHu = check19() && check3Men() && checkMenQing() == false && MJPlayerCard::isHoldCardCanHu(nJiang) && checkJianKePiao() && checkKezi();
	if (bCanHu) {
		m_vLastHuFanxing.clear();
		m_nLastHuCnt = 1;
		getFanXingAndFanCnt(m_vLastHuFanxing, m_nLastHuCnt);
	}
	setHuCard(0);
	return bCanHu;
}

void AHMJPlayerCard::getFanXingAndFanCnt(std::vector<eFanxingType>& vHuTypes, uint8_t& nFanCnt) {
	m_pFanxingChecker.checkFanxing(vHuTypes, this, 0, nullptr);
	sortFanCnt(vHuTypes, nFanCnt);
}

void AHMJPlayerCard::sortFanCnt(std::vector<eFanxingType>& vHuTypes, uint8_t& nFanCnt) {
	for (auto& ref : vHuTypes) {
		switch (ref)
		{
		case eFanxing_DuiDuiHu:
		{
			nFanCnt += 2;
		}
		break;
		case eFanxing_JiaHu:
		{
			nFanCnt += 1;
		}
		break;
		}
	}
}

bool AHMJPlayerCard::checkKezi() {
	VEC_CARD vTemp;
	if (getPengedCard(vTemp)) {
		return true;
	}
	else if (getMingGangedCard(vTemp)) {
		return true;
	}
	else if (getAnGangedCard(vTemp)) {
		return true;
	}
	else {
		auto vJianCard = m_vCards[eCT_Jian];
		for (uint8_t i = 0; i < vJianCard.size(); i++) {
			if (i + 1 < vJianCard.size() && vJianCard[i] == vJianCard[i + 1]) {
				return true;
			}
		}

		bool flag = false;
		for (uint8_t nType = eCT_Wan; nType < eCT_Jian; nType++) {
			auto vCard = m_vCards[nType];
			for (uint8_t i = 0; i < vCard.size(); i++) {
				auto nCard = vCard[i];
				if (i + 2 < vCard.size() && nCard == vCard[i + 2]) {
					removeHoldCard(nCard);
					removeHoldCard(nCard);
					removeHoldCard(nCard);
					uint8_t nJiang = 0;
					if (MJPlayerCard::isHoldCardCanHu(nJiang)) {
						flag = true;
					}
					addHoldCard(nCard);
					addHoldCard(nCard);
					addHoldCard(nCard);
					if (flag) {
						return true;
					}
					i += 2;
				}
			}
		}
		return false;
	}
}

bool AHMJPlayerCard::check3Men() {
	VEC_CARD vTemp;
	getEatedCard(vTemp);
	getPengedCard(vTemp);
	getAnGangedCard(vTemp);
	getMingGangedCard(vTemp);

	uint8_t nTypeCnt = 0;

	nTypeCnt += (m_vCards[eCT_Wan].size() > 0 || std::find_if(vTemp.begin(), vTemp.end(), [](uint8_t tCard) {
		return card_Type(tCard) == eCT_Wan;
	}) != vTemp.end()) ? 1 : 0;

	nTypeCnt += (m_vCards[eCT_Tong].size() > 0 || std::find_if(vTemp.begin(), vTemp.end(), [](uint8_t tCard) {
		return card_Type(tCard) == eCT_Tong;
	}) != vTemp.end()) ? 1 : 0;

	nTypeCnt += (m_vCards[eCT_Tiao].size() > 0 || std::find_if(vTemp.begin(), vTemp.end(), [](uint8_t tCard) {
		return card_Type(tCard) == eCT_Tiao;
	}) != vTemp.end()) ? 1 : 0;

	if (nTypeCnt == 3) {
		return true;
	}
	return false;
}

bool AHMJPlayerCard::check19() {
	VEC_CARD vTemp;
	getEatedCard(vTemp);
	getPengedCard(vTemp);
	getAnGangedCard(vTemp);
	getMingGangedCard(vTemp);

	if (std::find_if(vTemp.begin(), vTemp.end(), [](uint8_t &tCard) {
		auto tType = card_Type(tCard);
		if (eCT_Feng == tType || eCT_Jian == tType) {
			return true;
		}
		auto tValue = card_Value(tCard);
		if (tValue == 1 || tValue == 9) {
			return true;
		}
		return false;
	}) != vTemp.end()) {
		return true;
	}

	if (m_vCards[eCT_Feng].size() > 0 || m_vCards[eCT_Jian].size() > 0) {
		return true;
	}

	for (uint8_t nType = eCT_Wan; nType < eCT_Feng; nType++) {
		if (std::find_if(m_vCards[nType].begin(), m_vCards[nType].end(), [](uint8_t &tCard) {
			auto tValue = card_Value(tCard);
			if (tValue == 1 || tValue == 9) {
				return true;
			}
			return false;
		}) != m_vCards[nType].end()) {
			return true;
		}
	}

	return false;
}

bool AHMJPlayerCard::checkMenQing() {
	AHMJFanxingMenQing mq;
	return mq.checkFanxing(this, nullptr, 0, nullptr);
}

bool AHMJPlayerCard::checkJianKePiao() {
	VEC_CARD vTemp, vCards;
	getPengedCard(vTemp);
	vCards.insert(vCards.end(), m_vCards[eCT_Jian].begin(), m_vCards[eCT_Jian].end());

	bool haveJianKe = false;
	for (uint8_t i_value = 1; i_value < 4; i_value++) {
		auto iCard = make_Card_Num(eCT_Jian, i_value);
		if (std::find(vTemp.begin(), vTemp.end(), iCard) == vTemp.end() &&
			std::count(vCards.begin(), vCards.end(), iCard) < 3) {
			continue;
		}
		haveJianKe = true;
		break;
	}
	if (haveJianKe && checkDuiDui() == false) {
		return false;
	}
	return true;
}

bool AHMJPlayerCard::checkDuiDui() {
	FanxingDuiDuiHu fdd;
	return fdd.checkFanxing(this, nullptr, 0, nullptr);
}

bool AHMJPlayerCard::isJiaHu() {
	if (getHoldCardCnt() < 5) {
		return false;
	}
	auto nCard = getHuCard();
	if (isHaveCard(nCard) == false) {
		return false;
	}
	auto nType = card_Type(nCard);
	if (nType != eCT_Tiao && nType != eCT_Tong && nType != eCT_Wan) {
		return false;
	}
	auto nValue = card_Value(nCard);
	if (nValue == 1 || nValue == 9) {
		return false;
	}
	auto vCards = m_vCards[nType];
	if (vCards.size() < 3) {
		return false;
	}
	uint8_t nCnt = std::count_if(vCards.begin(), vCards.end(), [nCard](uint8_t& tCard) {
		return nCard == tCard;
	});
	auto nCardPre = make_Card_Num(nType, nValue - 1);
	uint8_t nCntPre = std::count_if(vCards.begin(), vCards.end(), [nCardPre](uint8_t& tCard) {
		return nCardPre == tCard;
	});
	if (nCntPre < 1) {
		return false;
	}
	auto nCardAfter = make_Card_Num(nType, nValue + 1);
	uint8_t nCntAfter = std::count_if(vCards.begin(), vCards.end(), [nCardAfter](uint8_t& tCard) {
		return nCardAfter == tCard;
	});
	if (nCntAfter < 1) {
		return false;
	}

	bool bFlag = false;
	auto nJiang = m_nJIang;
	removeHoldCard(nCard);
	removeHoldCard(nCardPre);
	removeHoldCard(nCardAfter);
	bFlag = MJPlayerCard::isHoldCardCanHu(m_nJIang);
	addHoldCard(nCard);
	addHoldCard(nCardPre);
	addHoldCard(nCardAfter);
	m_nJIang = nJiang;
	return bFlag;
}

bool AHMJPlayerCard::canHuOnlyOneCard() {
	if (getHoldCardCnt() < 3) {
		return true;
	}

	//TODO...
	auto nHuCard = getHuCard();
	if (isHaveCard(nHuCard) == false) {
		//m_bCanHuOnlyOne = false;
		return false;
	}
	auto nJiang = m_nJIang;
	removeHoldCard(nHuCard);

	bool bBreak = false;
	for (uint8_t nType = eCT_None; nType < eCT_Max; nType++) {
		auto vCards = m_vCards[nType];
		if (vCards.empty()) {
			continue;
		}
		uint8_t nMaxValue = 0;
		if (nType == eCT_Feng) {
			nMaxValue = 4;
		}
		else if (nType == eCT_Jian) {
			nMaxValue = 3;
		}
		else if (nType == eCT_Tiao || nType == eCT_Tong || nType == eCT_Wan) {
			nMaxValue = 9;
		}
		else {
			continue;
		}

		for (uint8_t nValue = 1; nValue <= nMaxValue; nValue++) {
			auto nCard = make_Card_Num((eMJCardType)nType, nValue);
			if (nCard == nHuCard) {
				continue;
			}
			if (canHuWitCard(nCard)) {
				bBreak = true;
				break;
			}
		}

		if (bBreak) {
			break;
		}
	}

	addHoldCard(nHuCard);
	m_nJIang = nJiang;

	return bBreak == false;
}

uint8_t AHMJPlayerCard::getHoldCardCnt() {
	VEC_CARD vHoldCard;
	getHoldCard(vHoldCard);
	return vHoldCard.size();
}

uint8_t AHMJPlayerCard::getHoldCardCnt(uint8_t nCard) {
	auto nType = card_Type(nCard);
	if (nType > eCT_None && nType < eCT_Max) {
		auto& vCard = m_vCards[nType];
		return std::count_if(vCard.begin(), vCard.end(), [nCard](const uint8_t tCard) {
			return tCard == nCard;
		});
	}
	return 0;
}