#include "SCMJPlayerCard.h"
#include "IMJPoker.h"
#include "log4z.h"

void SCMJPlayerCard::endDoHu(uint8_t nHuCard) {
	if (isHaveCard(nHuCard)) {
		removeHoldCard(nHuCard);
	}
	else {
		LOGFMTE("big error!!! do not have card = %u , end do hu failed", nHuCard);
	}
}

void SCMJPlayerCard::reset() {
	MJPlayerCard::reset();
	clearMiss();
	m_vHuCards.clear();
}

bool SCMJPlayerCard::canPengWithCard(uint8_t nCard) {
	if (m_vHuCards.size()) {
		return false;
	}
	auto nType = card_Type(nCard);
	if (nType == getMissType()) {
		return false;
	}
	return MJPlayerCard::canPengWithCard(nCard);
}

bool SCMJPlayerCard::canMingGangWithCard(uint8_t nCard) {
	auto nType = card_Type(nCard);
	if (nType == getMissType()) {
		return false;
	}
	bool flag = MJPlayerCard::canMingGangWithCard(nCard);
	if (flag && m_vHuCards.size()) {
		removeHoldCard(nCard);
		removeHoldCard(nCard);
		removeHoldCard(nCard);
		for (auto& ref : m_vHuCards) {
			flag = canHuWitCard(ref);
			if (flag) {
				continue;
			}
			break;
		}
		addHoldCard(nCard);
		addHoldCard(nCard);
		addHoldCard(nCard);
	}
	return flag;
}

bool SCMJPlayerCard::canAnGangWithCard(uint8_t nCard) {
	auto nType = card_Type(nCard);
	if (nType == getMissType()) {
		return false;
	}
	bool flag = MJPlayerCard::canAnGangWithCard(nCard);
	if (flag && m_vHuCards.size()) {
		if (getNewestFetchedCard() == nCard) {
			removeHoldCard(nCard);
			removeHoldCard(nCard);
			removeHoldCard(nCard);
			removeHoldCard(nCard);
			for (auto& ref : m_vHuCards) {
				flag = canHuWitCard(ref);
				if (flag) {
					continue;
				}
				break;
			}
			addHoldCard(nCard);
			addHoldCard(nCard);
			addHoldCard(nCard);
			addHoldCard(nCard);
		}
		else {
			flag = false;
		}
	}
	return flag;
}

bool SCMJPlayerCard::canBuGangWithCard(uint8_t nCard) {
	auto nType = card_Type(nCard);
	if (nType == getMissType()) {
		return false;
	}
	bool flag = MJPlayerCard::canBuGangWithCard(nCard);
	if (flag && m_vHuCards.size()) {
		if (getNewestFetchedCard() != nCard) {
			flag = false;
		}
	}
	return flag;
}

void SCMJPlayerCard::onVisitPlayerCardInfo(Json::Value& js, bool isSelf) {
	MJPlayerCard::onVisitPlayerCardInfo(js, isSelf);
	if (m_vHuCards.size()) {
		Json::Value jsHuCard;
		for (auto& ref : m_vHuCards) {
			jsHuCard[jsHuCard.size()] = ref;
		}
		js["huCards"] = jsHuCard;
	}
}

bool SCMJPlayerCard::getHoldCardThatCanAnGang(VEC_CARD& vGangCards) {
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

bool SCMJPlayerCard::getHoldCardThatCanBuGang(VEC_CARD& vGangCards) {
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

bool SCMJPlayerCard::isHoldCardCanHu() {
	uint8_t nJiang = 0;
	return isHoldCardCanHu(nJiang);
}

bool SCMJPlayerCard::isHoldCardCanHu(uint8_t& nJiang) {
	if (m_vCards[getMissType()].size()) {
		return false;
	}
	return MJPlayerCard::isHoldCardCanHu(nJiang);
}

bool SCMJPlayerCard::isTingPai() {
	if (m_vCards[getMissType()].size()) {
		return false;
	}
	return MJPlayerCard::isTingPai();
}

bool SCMJPlayerCard::isHoldCardCanHu(VEC_CARD vTemp) {
	VEC_CARD vAllCards[eCT_Max];
	for (auto nCard : vTemp)
	{
		auto eType = card_Type(nCard);
		if (eType > eCT_None || eType < eCT_Max)
		{
			vAllCards[eType].push_back(nCard);
		}
	}
	return isHoldCardCanHu(vAllCards);
}

bool SCMJPlayerCard::isHoldCardCanHu(VEC_CARD vTemp[eCT_Max]) {
	VEC_CARD vAllCards[eCT_Max];
	for (uint8_t i = eCT_None; i < eCT_Max; i++) {
		vAllCards[i] = m_vCards[i];
		m_vCards[i].clear();
		m_vCards[i] = vTemp[i];
	}
	uint8_t nJiang = 0;
	bool canHu = isHoldCardCanHu(nJiang);
	for (uint8_t i = eCT_None; i < eCT_Max; i++) {
		m_vCards[i].clear();
		m_vCards[i] = vAllCards[i];
	}
	return canHu;
}

bool SCMJPlayerCard::onExchageCards(VEC_CARD vOwn, VEC_CARD vExchage) {
	if (vOwn.size() != vExchage.size()) {
		return false;
	}

	if (isHaveCards(vOwn) == false) {
		return false;
	}

	for (auto ref : vOwn) {
		removeHoldCard(ref);
	}

	for (auto ref : vExchage) {
		addHoldCard(ref);
	}

	return true;
}

void SCMJPlayerCard::onAutoDecideExchageCards(uint8_t nAmount, VEC_CARD& vCards) {
	uint8_t nType = eCT_None;
	uint8_t nTypeAmount = 0;
	for (uint8_t i = eCT_Wan; i < eCT_Feng; i++) {
		if (m_vCards[i].size() < nAmount) {
			continue;
		}
		if (nTypeAmount && nTypeAmount <= m_vCards[i].size()) {
			continue;
		}
		nType = i;
		nTypeAmount = m_vCards[i].size();
	}
	if (nType < eCT_Wan || nType > eCT_Tiao) {
		LOGFMTE("big error!!! Can not auto find exchange cards.");
		return;
	}
	for (uint8_t i = 0; i < nAmount; i++) {
		vCards.push_back(m_vCards[nType][i]);
	}
}

void SCMJPlayerCard::onAutoSetMiss() {
	uint8_t nType = eCT_Wan;
	uint8_t nTypeAmount = 0;
	for (uint8_t i = eCT_Wan; i < eCT_Feng; i++) {
		if (nTypeAmount && nTypeAmount <= m_vCards[i].size()) {
			continue;
		}
		nType = i;
		nTypeAmount = m_vCards[i].size();
	}
	setMiss((eMJCardType)nType);
}

bool SCMJPlayerCard::isHaveCards(VEC_CARD vCards) {
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

bool SCMJPlayerCard::isHuaZhu() {
	return m_vCards[getMissType()].size() > 0;
}

void SCMJPlayerCard::getQueType(std::vector<eMJCardType>& vType) {
	for (uint8_t i = eCT_Wan; i < eCT_Feng; i++) {
		uint8_t nAmount = m_vCards[i].size();
		if (nAmount == 0 || nAmount % 3 == 0) {
			continue;
		}
		vType.push_back((eMJCardType)i);
	}
}

bool SCMJPlayerCard::eraseVector(uint8_t p, VEC_CARD& typeVec)
{
	VEC_CARD::iterator it = find(typeVec.begin(), typeVec.end(), p);
	if (it != typeVec.end())
	{
		typeVec.erase(it);
		return true;
	}
	return false;
}