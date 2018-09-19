#include "FXMJPlayerCard.h"
#include "IMJPoker.h"
#include "log4z.h"
#include "FanxingDuiDuiHu.h"

void FXMJPlayerCard::reset() {
	MJPlayerCard::reset();
	m_nHuCard = 0;
	m_bCanHuOnlyOne = false;
	m_bCheckedCanHuOnlyOne = false;
	clearTing();
	clearPreGang();
	m_nLouHuLevel = 0;
}

bool FXMJPlayerCard::canEatCard(uint8_t nCard, uint8_t nWithA, uint8_t nWithB) {
	if (isTing()) {
		return false;
	}
	/*VEC_CARD vCards;
	getHoldCard(vCards);
	if (vCards.size() < 5) {
		return false;
	}*/
	return MJPlayerCard::canEatCard(nCard, nWithA, nWithB);
}

bool FXMJPlayerCard::canPengWithCard(uint8_t nCard) {
	if (isTing()) {
		return false;
	}
	/*VEC_CARD vHoldCards;
	getHoldCard(vHoldCards);
	if (vHoldCards.size() < 5) {
		VEC_CARD vTemp;
		getEatedCard(vTemp);
		if (vTemp.size() > 0) {
			return false;
		}
	}*/
	return MJPlayerCard::canPengWithCard(nCard);
}

bool FXMJPlayerCard::canMingGangWithCard(uint8_t nCard) {
	/*VEC_CARD vHoldCards;
	getHoldCard(vHoldCards);
	if (vHoldCards.size() < 5) {
		VEC_CARD vTemp;
		getEatedCard(vTemp);
		if (vTemp.size() > 0) {
			return false;
		}
	}*/
	bool flag = MJPlayerCard::canMingGangWithCard(nCard);
	if (flag) {
		if (isTing()) {
			removeHoldCard(nCard);
			removeHoldCard(nCard);
			removeHoldCard(nCard);
			flag = tingCheck(nCard);
			addHoldCard(nCard);
			addHoldCard(nCard);
			addHoldCard(nCard);
		}
	}
	return flag;
}

bool FXMJPlayerCard::canAnGangWithCard(uint8_t nCard) {
	/*VEC_CARD vHoldCards;
	getHoldCard(vHoldCards);
	if (vHoldCards.size() < 6) {
		VEC_CARD vTemp;
		getEatedCard(vTemp);
		if (vTemp.size() > 0) {
			return false;
		}
	}*/
	bool flag = checkMenQing() == false && MJPlayerCard::canAnGangWithCard(nCard);
	if (flag) {
		if (isTing()) {
			flag = getNewestFetchedCard() == nCard;
			if (flag) {
				removeHoldCard(nCard);
				removeHoldCard(nCard);
				removeHoldCard(nCard);
				removeHoldCard(nCard);
				flag = tingCheck(nCard);
				addHoldCard(nCard);
				addHoldCard(nCard);
				addHoldCard(nCard);
				addHoldCard(nCard);
			}
		}
	}
	return flag;
}

bool FXMJPlayerCard::canBuGangWithCard(uint8_t nCard) {
	bool flag = MJPlayerCard::canBuGangWithCard(nCard);
	if (flag) {
		if (isTing()) {
			flag = getNewestFetchedCard() == nCard;
		}
	}
	return flag;
}

void FXMJPlayerCard::onVisitPlayerCardInfo(Json::Value& js, bool isSelf) {
	MJPlayerCard::onVisitPlayerCardInfo(js, isSelf);
	js["ting"] = isTing() ? (isCool() ? 2 : 1) : 0;
}

bool FXMJPlayerCard::getHoldCardThatCanAnGang(VEC_CARD& vGangCards) {
	/*VEC_CARD vHoldCards;
	getHoldCard(vHoldCards);
	if (vHoldCards.size() < 6) {
		VEC_CARD vTemp;
		getEatedCard(vTemp);
		if (vTemp.size() > 0) {
			return false;
		}
	}*/

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

bool FXMJPlayerCard::getHoldCardThatCanBuGang(VEC_CARD& vGangCards) {
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

bool FXMJPlayerCard::canCycloneWithCard(uint8_t nCard) {
	if (isTing()) {
		return false;
	}
	/*VEC_CARD vHoldCards;
	getHoldCard(vHoldCards);
	if (vHoldCards.size() < 6) {
		VEC_CARD vTemp;
		getEatedCard(vTemp);
		if (vTemp.size() > 0) {
			return false;
		}
	}*/

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

bool FXMJPlayerCard::getHoldCardThatCanCyclone(VEC_CARD& vGangCards) {
	if (isTing()) {
		return false;
	}
	/*VEC_CARD vHoldCards;
	getHoldCard(vHoldCards);
	if (vHoldCards.size() < 6) {
		VEC_CARD vTemp;
		getEatedCard(vTemp);
		if (vTemp.size() > 0) {
			return false;
		}
	}*/

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

bool FXMJPlayerCard::onDirectGang(uint8_t nCard, uint8_t nGangGetCard, uint16_t nInvokerIdx) {
	bool flag = MJPlayerCard::onDirectGang(nCard, nGangGetCard, nInvokerIdx);
	if (flag && isTing()) {
		VEC_CARD vCards;
		tingCheck(vCards);
		m_vTingCards.assign(vCards.begin(), vCards.end());
	}
	return flag;
}

bool FXMJPlayerCard::onDirectGang(uint8_t nCard, uint16_t nInvokerIdx) {
	auto eType = card_Type(nCard);
	if (eType >= eCT_Max)
	{
		LOGFMTE("onMingGang parse card type error so do not have this card = %u", nCard);
		return false;
	}

	auto& vCard = m_vCards[eType];
	auto nEraseCnt = 3;
	while (nEraseCnt-- > 0)
	{
		auto iter = std::find(vCard.begin(), vCard.end(), nCard);
		vCard.erase(iter);
	}

	// sign ming gang info 
	VEC_INVOKE_ACT_INFO::value_type tMingGang;
	tMingGang.nTargetCard = nCard;
	tMingGang.nInvokerIdx = (uint8_t)nInvokerIdx;
	tMingGang.eAct = eMJAct_MingGang;
	m_vMingCardInfo.push_back(tMingGang);

	//debugCardInfo();

	if (isTing()) {
		VEC_CARD vCards;
		tingCheck(vCards);
		m_vTingCards.assign(vCards.begin(), vCards.end());
	}
	return true;
}

bool FXMJPlayerCard::onAnGang(uint8_t nCard, uint8_t nGangGetCard) {
	bool flag = MJPlayerCard::onAnGang(nCard, nGangGetCard);
	if (flag && isTing()) {
		VEC_CARD vCards;
		tingCheck(vCards);
		m_vTingCards.assign(vCards.begin(), vCards.end());
	}
	return flag;
}

bool FXMJPlayerCard::onAnGang(uint8_t nCard) {
	auto eType = card_Type(nCard);
	if (eType >= eCT_Max)
	{
		LOGFMTE("onAnGang parse card type error so do not have this card = %u", nCard);
		return false;
	}

	auto& vCard = m_vCards[eType];
	auto nEraseCnt = 4;
	while (nEraseCnt-- > 0)
	{
		auto iter = std::find(vCard.begin(), vCard.end(), nCard);
		vCard.erase(iter);
	}

	//addCardToVecAsc(m_vGanged, nCard);
	VEC_INVOKE_ACT_INFO::value_type tAnGang;
	tAnGang.nTargetCard = nCard;
	tAnGang.nInvokerIdx = 0;
	tAnGang.eAct = eMJAct_AnGang;
	m_vMingCardInfo.push_back(tAnGang);
	//debugCardInfo();

	if (isTing()) {
		VEC_CARD vCards;
		tingCheck(vCards);
		m_vTingCards.assign(vCards.begin(), vCards.end());
	}
	return true;
}

bool FXMJPlayerCard::onBuGang(uint8_t nCard) {
	auto eType = card_Type(nCard);
	if (eType >= eCT_Max)
	{
		LOGFMTE("onMingGang parse card type error so do not have this card = %u", nCard);
		return false;
	}

	// remove hold card 
	removeHoldCard(nCard);

	// remove peng 
	auto iterPeng = std::find_if(m_vMingCardInfo.begin(), m_vMingCardInfo.end(), [nCard](VEC_INVOKE_ACT_INFO::value_type& ref) { return ref.nTargetCard == nCard && ref.eAct == eMJAct_Peng; });
	if (iterPeng == m_vMingCardInfo.end())
	{
		LOGFMTE("not peng , hao to bu gang ? %u ", nCard);
		return false;
	}
	// sign Bu gang info 
	iterPeng->eAct = eMJAct_BuGang;

	//debugCardInfo();
	return true;
}

bool FXMJPlayerCard::onCyclone(uint8_t nCard, uint8_t nGangGetCard) {
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

bool FXMJPlayerCard::getCycloneCard(VEC_CARD& vGangCard)
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

bool FXMJPlayerCard::isHoldCardCanHu() {
	return std::find(m_vTingCards.begin(), m_vTingCards.end(), getNewestFetchedCard()) != m_vTingCards.end();
	//return isHoldCardCanHu(m_nJIang);
}

bool FXMJPlayerCard::isHoldCardCanHu(uint8_t& nJiang) {
	return std::find(m_vTingCards.begin(), m_vTingCards.end(), getNewestFetchedCard()) != m_vTingCards.end();
	/*if (check3Men() && check19() && checkKezi()) {
		return MJPlayerCard::isHoldCardCanHu(nJiang);
	}
	return false;*/
}

bool FXMJPlayerCard::canHuWitCard(uint8_t nCard) {
	if (m_bCool) {
		return false;
	}
	auto flag = std::find(m_vTingCards.begin(), m_vTingCards.end(), nCard) != m_vTingCards.end();
	if (flag) {
		if (m_nLouHuLevel) {
			if (m_nLouHuLevel == 3 || getHuLevel(nCard) < 3) {
				flag = false;
			}
		}
	}
	return flag;
}

bool FXMJPlayerCard::canRotDirectGang(uint8_t nCard) {
	if (m_bCool) {
		return false;
	}
	return std::find(m_vTingCards.begin(), m_vTingCards.end(), nCard) != m_vTingCards.end();
}

bool FXMJPlayerCard::checkCanHuWithCard(uint8_t nCard) {
	auto eType = card_Type(nCard);
	if (eType >= eCT_Max)
	{
		LOGFMTE("parse card type error ,canHuWitCard have this card = %u", nCard);
		return false;
	}

	addHoldCard(nCard);
	bool bSelfHu = checkHoldCardCanHu();
	removeHoldCard(nCard);
	//debugCardInfo();
	return bSelfHu;
}

bool FXMJPlayerCard::checkHoldCardCanHu() {
	uint8_t nJiang = 0;
	bool flag = MJPlayerCard::isHoldCardCanHu(nJiang);
	if (flag) {
		flag = check3Men() && check19() && checkSB1();
		if (flag && m_b7Pair == false) {
			flag = checkMenQing() == false && checkKezi();
		}
	}
	return flag;
}

bool FXMJPlayerCard::isTingPai() {
	bool flag = MJPlayerCard::isTingPai();
	if (flag) {
		VEC_CARD vTingCards;
		for (uint8_t nType = eCT_None; nType < eCT_Max; nType++) {
			if (nType < eCT_Max) {
				auto vCard = m_vCards[nType];
				if (vCard.empty()) {
					continue;
				}

				uint8_t nMax = 9;
				if (nType == eCT_Feng) {
					nMax = 4;
				}
				else if (nType == eCT_Jian) {
					nMax = 3;
				}

				for (uint8_t nValue = 1; nValue <= nMax; nValue++) {
					auto nCard = make_Card_Num((eMJCardType)nType, nValue);
					if (checkCanHuWithCard(nCard)) {
						vTingCards.push_back(nCard);
					}
				}
			}
		}
		flag = vTingCards.size();
		if (flag) {
			m_vTingCards.clear();
			m_vTingCards.assign(vTingCards.begin(), vTingCards.end());
		}
	}
	return flag;
}

bool FXMJPlayerCard::tingCheck(uint8_t nGangCard) {
	//TODO...
	if (m_vTingCards.empty()) {
		return false;
	}

	if (nGangCard) {
		addPreGang(nGangCard);
	}

	//VEC_CARD vCards;
	for (auto ref : m_vTingCards) {
		if (checkCanHuWithCard(ref)) {
			return true;
		}
	}

	clearPreGang();
	return false;
}

bool FXMJPlayerCard::tingCheck(VEC_CARD& vCards) {
	if (m_vTingCards.empty()) {
		return false;
	}

	for (auto ref : m_vTingCards) {
		if (checkCanHuWithCard(ref)) {
			vCards.push_back(ref);
		}
	}
	return vCards.empty() == false;
}

//bool FXMJPlayerCard::isHoldCardCanHu(VEC_CARD vTemp) {
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
//bool FXMJPlayerCard::isHoldCardCanHu(VEC_CARD vTemp[eCT_Max]) {
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

bool FXMJPlayerCard::isHaveCards(VEC_CARD vCards) {
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

bool FXMJPlayerCard::eraseVector(uint8_t p, VEC_CARD& typeVec)
{
	VEC_CARD::iterator it = find(typeVec.begin(), typeVec.end(), p);
	if (it != typeVec.end())
	{
		typeVec.erase(it);
		return true;
	}
	return false;
}

bool FXMJPlayerCard::checkKezi() {
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
	else if (m_vPreGang.size()) {
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

bool FXMJPlayerCard::check3Men() {
	VEC_CARD vTemp;
	getEatedCard(vTemp);
	getPengedCard(vTemp);
	getAnGangedCard(vTemp);
	getMingGangedCard(vTemp);
	getCycloneCard(vTemp);
	vTemp.insert(vTemp.end(), m_vPreGang.begin(), m_vPreGang.end());

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
	else if (isEnableOOT() && nTypeCnt < 2) {
		return true;
	}
	return false;
}

bool FXMJPlayerCard::check19() {
	VEC_CARD vTemp;
	getEatedCard(vTemp);
	getPengedCard(vTemp);
	getAnGangedCard(vTemp);
	getMingGangedCard(vTemp);
	getCycloneCard(vTemp);
	vTemp.insert(vTemp.end(), m_vPreGang.begin(), m_vPreGang.end());

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

bool FXMJPlayerCard::checkMenQing() {
	VEC_CARD vTemp;
	getEatedCard(vTemp);
	getPengedCard(vTemp);
	getMingGangedCard(vTemp);
	return vTemp.empty();
}

bool FXMJPlayerCard::checkSB1() {
	auto nCnt = getHoldCardCnt();
	if (nCnt < 3) {
		if (isEnableSB1() == false) {
			FanxingDuiDuiHu fdd;
			return fdd.checkFanxing(this, nullptr, 0, nullptr);
		}
	}
	return true;
}

bool FXMJPlayerCard::canHuOnlyOneCard() {
	/*if (m_bCheckedCanHuOnlyOne) {
		return m_bCanHuOnlyOne;
	}*/

	if (getHoldCardCnt() < 3) {
		/*m_bCanHuOnlyOne = true;
		m_bCheckedCanHuOnlyOne = true;*/
		return true;
	}

	//TODO...
	auto nHuCard = getHuCard();
	if (isHaveCard(nHuCard) == false) {
		//m_bCanHuOnlyOne = false;
		return false;
	}

	return m_vTingCards.size() == 1;
	/*auto nJiang = m_nJIang;
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
	return m_bCanHuOnlyOne;*/
}

uint8_t FXMJPlayerCard::getHoldCardCnt() {
	VEC_CARD vHoldCard;
	getHoldCard(vHoldCard);
	return vHoldCard.size();
}

uint8_t FXMJPlayerCard::getHoldCardCnt(uint8_t nCard) {
	auto nType = card_Type(nCard);
	if (nType > eCT_None && nType < eCT_Max) {
		auto& vCard = m_vCards[nType];
		return std::count_if(vCard.begin(), vCard.end(), [nCard](const uint8_t tCard) {
			return tCard == nCard;
		});
	}
	return 0;
}

void FXMJPlayerCard::clearTing() {
	m_vTingCards.clear();
	m_bCool = false;
}

uint8_t FXMJPlayerCard::getHuLevel(uint8_t nHuCard) {
	uint8_t nHuLevel = 1;
	addHoldCard(nHuCard);
	FanxingDuiDuiHu fdd;
	if (fdd.checkFanxing(this, nullptr, 0, nullptr)) {
		nHuLevel = 2;
		if (getHoldCardCnt(nHuCard) == 2) {
			nHuLevel = 3;
		}
	}
	removeHoldCard(nHuCard);
	return nHuLevel;
}

void FXMJPlayerCard::onPlayerLouHu(uint8_t nCard) {
	if (std::find(m_vTingCards.begin(), m_vTingCards.end(), nCard) != m_vTingCards.end()) {
		m_nLouHuLevel = getHuLevel(nCard);
	}
}