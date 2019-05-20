#include "DDMJPlayerCard.h"
#include "IMJPoker.h"
#include "log4z.h"

void DDMJPlayerCard::reset() {
	MJPlayerCard::reset();
	m_nHuCard = 0;
	m_bCanHuOnlyOne = false;
	m_bCheckedCanHuOnlyOne = false;
}

bool DDMJPlayerCard::canEatCard(uint8_t nCard, uint8_t nWithA, uint8_t nWithB) {
	VEC_CARD vCards;
	getHoldCard(vCards);
	if (vCards.size() < 5) {
		return false;
	}
	return MJPlayerCard::canEatCard(nCard, nWithA, nWithB);
}

bool DDMJPlayerCard::canPengWithCard(uint8_t nCard) {
	VEC_CARD vHoldCards;
	getHoldCard(vHoldCards);
	if (vHoldCards.size() < 5) {
		VEC_CARD vTemp;
		getEatedCard(vTemp);
		if (vTemp.size() > 0) {
			return false;
		}
	}
	return MJPlayerCard::canPengWithCard(nCard);
}

bool DDMJPlayerCard::canMingGangWithCard(uint8_t nCard) {
	VEC_CARD vHoldCards;
	getHoldCard(vHoldCards);
	if (vHoldCards.size() < 5) {
		VEC_CARD vTemp;
		getEatedCard(vTemp);
		if (vTemp.size() > 0) {
			return false;
		}
	}
	return MJPlayerCard::canMingGangWithCard(nCard);
}

bool DDMJPlayerCard::canAnGangWithCard(uint8_t nCard) {
	VEC_CARD vHoldCards;
	getHoldCard(vHoldCards);
	if (vHoldCards.size() < 6) {
		VEC_CARD vTemp;
		getEatedCard(vTemp);
		if (vTemp.size() > 0) {
			return false;
		}
	}
	return MJPlayerCard::canAnGangWithCard(nCard);
}

void DDMJPlayerCard::onVisitPlayerCardInfo(Json::Value& js, bool isSelf) {
	MJPlayerCard::onVisitPlayerCardInfo(js, isSelf);
}

bool DDMJPlayerCard::getHoldCardThatCanAnGang(VEC_CARD& vGangCards) {
	VEC_CARD vHoldCards;
	getHoldCard(vHoldCards);
	if (vHoldCards.size() < 6) {
		VEC_CARD vTemp;
		getEatedCard(vTemp);
		if (vTemp.size() > 0) {
			return false;
		}
	}

	for (auto& vCard : m_vCards)
	{
		if (vCard.size() < 4)
		{
			continue;
		}

		for (uint8_t nIdx = 0; (uint8_t)(nIdx + 3) < vCard.size();)
		{
			if (vCard[nIdx] == vCard[nIdx + 3])
			{
				if (canAnGangWithCard(vCard[nIdx])) {
					vGangCards.push_back(vCard[nIdx]);
				}
				nIdx += 4;
			}
			else
			{
				++nIdx;
			}
		}
	}
	return !vGangCards.empty();
}

bool DDMJPlayerCard::getHoldCardThatCanBuGang(VEC_CARD& vGangCards) {
	for (auto& ref : m_vMingCardInfo)
	{
		if (ref.eAct != eMJAct_Peng)
		{
			continue;
		}

		if (canBuGangWithCard(ref.nTargetCard))
		{
			vGangCards.push_back(ref.nTargetCard);
		}
	}

	return !vGangCards.empty();
}

bool DDMJPlayerCard::canCycloneWithCard(uint8_t nCard) {
	VEC_CARD vHoldCards;
	getHoldCard(vHoldCards);
	if (vHoldCards.size() < 6) {
		VEC_CARD vTemp;
		getEatedCard(vTemp);
		if (vTemp.size() > 0) {
			return false;
		}
	}

	auto nType = card_Type(nCard);
	if (nType != eCT_Feng && nType != eCT_Jian) {
		return false;
	}
	auto vCard = m_vCards[nType];
	uint8_t nNeed = nType == eCT_Feng ? 4 : 3;
	if (vCard.size() < nNeed) {
		return false;
	}
	bool isBreak = false;
	for (uint8_t nValue = 1; nValue <= nNeed; nValue++) {
		if (isHaveCard(make_Card_Num(nType, nValue)) == false) {
			isBreak = true;
			break;
		}
	}
	if (isBreak) {
		return false;
	}
	if (nType == eCT_Jian && isHaveCard(make_Card_Num(eCT_Tiao, 1)) == false) {
		return false;
	}
	return true;
}

bool DDMJPlayerCard::getHoldCardThatCanCyclone(VEC_CARD& vGangCards) {
	VEC_CARD vHoldCards;
	getHoldCard(vHoldCards);
	if (vHoldCards.size() < 6) {
		VEC_CARD vTemp;
		getEatedCard(vTemp);
		if (vTemp.size() > 0) {
			return false;
		}
	}

	for (uint8_t i = eCT_None; i < eCT_Max; i++)
	{
		//auto vCard = m_vCards[i];
		if (i == eCT_Feng || i == eCT_Jian) {
			auto vCard = m_vCards[i];
			uint8_t nNeed = i == eCT_Feng ? 4 : 3;
			if (vCard.size() < nNeed) {
				continue;
			}
			bool isBreak = false;
			for (uint8_t nValue = 1; nValue <= nNeed; nValue++) {
				if (isHaveCard(make_Card_Num((eMJCardType)i, nValue)) == false) {
					isBreak = true;
					break;
				}
			}
			if (isBreak) {
				continue;
			}
			if (i == eCT_Jian && isHaveCard(make_Card_Num(eCT_Tiao, 1)) == false) {
				continue;
			}
			vGangCards.push_back(make_Card_Num((eMJCardType)i, 1));
		}
	}
	return !vGangCards.empty();
}

bool DDMJPlayerCard::onCyclone(uint8_t nCard, uint8_t nGangGetCard) {
	auto eType = card_Type(nCard);
	if (eType != eCT_Feng && eType != eCT_Jian)
	{
		LOGFMTE("onCyclone parse card type error so can not cyclone this card = %u", nCard);
		return false;
	}
	uint8_t nNeed = eType == eCT_Feng ? 4 : 3;
	auto& vCard = m_vCards[eType];
	if (vCard.size() < nNeed) {
		LOGFMTE("onCyclone error so can not cyclone this card = %u", nCard);
		return false;
	}
	bool isBreak = false;
	for (uint8_t nValue = 1; nValue <= nNeed; nValue++) {
		if (isHaveCard(make_Card_Num((eMJCardType)eType, nValue)) == false) {
			isBreak = true;
			break;
		}
	}
	if (isBreak) {
		LOGFMTE("onCyclone error so can not cyclone this card = %u", nCard);
		return false;
	}
	if (eType == eCT_Jian && isHaveCard(make_Card_Num(eCT_Tiao, 1)) == false) {
		LOGFMTE("onCyclone error has no 1ji so can not cyclone this card = %u", nCard);
		return false;
	}

	while (nNeed > 0)
	{
		auto iter = std::find(vCard.begin(), vCard.end(), make_Card_Num((eMJCardType)eType, nNeed));
		vCard.erase(iter);
		nNeed--;
	}
	if (eType == eCT_Jian) {
		removeHoldCard(make_Card_Num(eCT_Tiao, 1));
		/*vCard = m_vCards[eCT_Tiao];
		auto iter = std::find(vCard.begin(), vCard.end(), make_Card_Num(eCT_Tiao, 1));
		vCard.erase(iter);*/
	}

	// sign cyclone gang info 
	VEC_INVOKE_ACT_INFO::value_type tMingGang;
	tMingGang.nTargetCard = nCard;
	tMingGang.nInvokerIdx = 0;
	tMingGang.eAct = eMJAct_Cyclone;
	m_vMingCardInfo.push_back(tMingGang);

	// new get card ;
	onMoCard(nGangGetCard);
	//debugCardInfo();
	return true;
}

bool DDMJPlayerCard::getCycloneCard(VEC_CARD& vGangCard)
{
	for (auto ref : m_vMingCardInfo)
	{
		if (ref.eAct == eMJAct_Cyclone)
		{
			vGangCard.push_back(ref.nTargetCard);
		}
	}
	return !vGangCard.empty();
}

bool DDMJPlayerCard::isHoldCardCanHu() {
	return isHoldCardCanHu(m_nJIang);
}

bool DDMJPlayerCard::isHoldCardCanHu(uint8_t& nJiang) {
	if (check3Men() && check19() && checkKezi()) {
		return MJPlayerCard::isHoldCardCanHu(nJiang);
	}
	return false;
}

bool DDMJPlayerCard::isTingPai() {
	return MJPlayerCard::isTingPai();
}

//bool DDMJPlayerCard::isHoldCardCanHu(VEC_CARD vTemp) {
//	VEC_CARD vAllCards[eCT_Max];
//	for (auto nCard : vTemp)
//	{
//		auto eType = card_Type(nCard);
//		if (eType > eCT_None || eType < eCT_Max)
//		{
//			vAllCards[eType].push_back(nCard);
//		}
//	}
//	return isHoldCardCanHu(vAllCards);
//}
//
//bool DDMJPlayerCard::isHoldCardCanHu(VEC_CARD vTemp[eCT_Max]) {
//	VEC_CARD vAllCards[eCT_Max];
//	for (uint8_t i = eCT_None; i < eCT_Max; i++) {
//		vAllCards[i] = m_vCards[i];
//		m_vCards[i].clear();
//		m_vCards[i] = vTemp[i];
//	}
//	uint8_t nJiang = 0;
//	bool canHu = isHoldCardCanHu(nJiang);
//	for (uint8_t i = eCT_None; i < eCT_Max; i++) {
//		m_vCards[i].clear();
//		m_vCards[i] = vAllCards[i];
//	}
//	return canHu;
//}

bool DDMJPlayerCard::isHaveCards(VEC_CARD vCards) {
	VEC_CARD vTemp;
	getHoldCard(vTemp);
	for (auto ref : vCards) {
		if (eraseVector(ref, vTemp)) {
			continue;
		}
		return false;
	}
	return true;
}

bool DDMJPlayerCard::eraseVector(uint8_t p, VEC_CARD& typeVec)
{
	VEC_CARD::iterator it = find(typeVec.begin(), typeVec.end(), p);
	if (it != typeVec.end())
	{
		typeVec.erase(it);
		return true;
	}
	return false;
}

bool DDMJPlayerCard::checkKezi() {
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
	else if (getCycloneCard(vTemp)) {
		return true;
	}
	else {
		bool flag = false;
		for (uint8_t nType = eCT_Wan; nType <= eCT_Jian; nType++) {
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

bool DDMJPlayerCard::check3Men() {
	VEC_CARD vTemp;
	getEatedCard(vTemp);
	getPengedCard(vTemp);
	getAnGangedCard(vTemp);
	getMingGangedCard(vTemp);
	getCycloneCard(vTemp);

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
	/*else if (m_pCurRoom->isQiYiSe() && nTypeCnt < 2) {
		return true;
	}*/
	return false;
}

bool DDMJPlayerCard::check19() {
	VEC_CARD vTemp;
	getEatedCard(vTemp);
	getPengedCard(vTemp);
	getAnGangedCard(vTemp);
	getMingGangedCard(vTemp);
	getCycloneCard(vTemp);

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

bool DDMJPlayerCard::isJiaHu() {
	if (getHoldCardCnt() < 5) {
		return false;
	}
	/*if (canHuOnlyOneCard() == false) {
		return false;
	}*/
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

bool DDMJPlayerCard::isDanDiao() {
	if (getHoldCardCnt() < 3) {
		return true;
	}
	/*if (canHuOnlyOneCard() == false) {
		return false;
	}*/
	auto nCard = getHuCard();
	if (isHaveCard(nCard) == false) {
		return false;
	}
	/*if (m_nJIang != nCard) {
		return false;
	}*/
	auto nType = card_Type(nCard);
	auto vCards = m_vCards[nType];

	uint8_t nCnt = std::count_if(vCards.begin(), vCards.end(), [nCard](uint8_t& tCard) {
		return nCard == tCard;
	});
	if (nCnt < 2) {
		return false;
	}
	if (m_nJIang == nCard) {
		return true;
	}
	bool bFlag = false;
	removeHoldCard(nCard);
	removeHoldCard(nCard);
	bFlag = CheckHoldCardAllShun();
	addHoldCard(nCard);
	addHoldCard(nCard);
	return bFlag;
}

bool DDMJPlayerCard::isBianHu() {
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
	if (nValue != 7 && nValue != 3) {
		return false;
	}
	auto vCards = m_vCards[nType];
	if (vCards.size() < 3) {
		return false;
	}
	uint8_t nCnt = std::count_if(vCards.begin(), vCards.end(), [nCard](uint8_t& tCard) {
		return nCard == tCard;
	});
	auto nCardPre = make_Card_Num(nType, nValue == 3 ? nValue - 1 : nValue + 1);
	uint8_t nCntPre = std::count_if(vCards.begin(), vCards.end(), [nCardPre](uint8_t& tCard) {
		return nCardPre == tCard;
	});
	if (nCntPre < 1) {
		return false;
	}
	auto nCardAfter = make_Card_Num(nType, nValue == 3 ? nValue - 2 : nValue + 2);
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
	return bFlag;
}

bool DDMJPlayerCard::canHuOnlyOneCard() {
	if (m_bCheckedCanHuOnlyOne) {
		return m_bCanHuOnlyOne;
	}

	if (getHoldCardCnt() < 3) {
		m_bCanHuOnlyOne = true;
		m_bCheckedCanHuOnlyOne = true;
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
	if (bBreak == false) {
		m_bCanHuOnlyOne = true;
	}

	addHoldCard(nHuCard);
	m_nJIang = nJiang;

	m_bCheckedCanHuOnlyOne = true;
	return m_bCanHuOnlyOne;
}

bool DDMJPlayerCard::isHuOnly19() {
	auto nCard = getHuCard();
	if (isHaveCard(nCard) == false) {
		return false;
	}
	auto nType = card_Type(nCard);
	if (nType > eCT_None && nType < eCT_Feng) {
		bool flag = false;
		auto nValue = card_Value(nCard);
		if (nValue == 1 || nValue == 9) {
			removeHoldCard(nCard);
			flag = check19() == false;
			addHoldCard(nCard);
		}
		return flag;
	}

	return false;
}

uint8_t DDMJPlayerCard::getHoldCardCnt() {
	VEC_CARD vHoldCard;
	getHoldCard(vHoldCard);
	return vHoldCard.size();
}