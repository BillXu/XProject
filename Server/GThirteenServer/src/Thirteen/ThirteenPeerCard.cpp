#include "ThirteenPeerCard.h"
#include "log4z.h"
#include <algorithm>
#include "ThirteenCardTypeChecker.h"
ThirteenPeerCard::ThirteenPeerCard(){
	m_nCurGroupIdx = DAO_HEAD;
	m_vGroups.resize(DAO_MAX);
	for (auto& group : m_vGroups) {
		group.reset();
	}
	m_vHoldCards.clear();
}

void ThirteenPeerCard::setOpts()
{
}

void ThirteenPeerCard::addCompositCardNum( uint8_t nCardCompositNum )
{
	if (m_vHoldCards.size() < MAX_HOLD_CARD_COUNT)
	{
		m_vHoldCards.push_back(nCardCompositNum);
	}
	else
	{
		LOGFMTE("hold card is overflow , can not add more");
	}
}

bool ThirteenPeerCard::setDao(uint8_t nIdx, VEC_CARD vCards) {
	if (nIdx < DAO_MAX) {
		std::sort(vCards.begin(), vCards.end());
		std::reverse(vCards.begin(), vCards.end());
		return m_vGroups[nIdx].setCard(vCards);
	}
	return false;
}

bool ThirteenPeerCard::autoSetDao() {
	if (m_vHoldCards.size() == MAX_HOLD_CARD_COUNT) {
		VEC_CARD vCards;
		vCards.assign(m_vHoldCards.begin(), m_vHoldCards.end());
		std::sort(vCards.begin(), vCards.end());
		std::reverse(vCards.begin(), vCards.end());
		for (uint8_t nDao = DAO_MAX; nDao > DAO_HEAD; nDao--) {
			if (autoSetDao(vCards, nDao - 1)) {
				continue;
			}
			return false;
		}
		return true;
	}
	return false;
}

bool ThirteenPeerCard::reSetDao() {
	for (auto& ref : m_vGroups) {
		ref.reset();
	}
	return true;
}

const char*  ThirteenPeerCard::getNameString()
{
	return "thirteen" ;
}

uint8_t ThirteenPeerCard::getType()
{
	return m_vGroups[m_nCurGroupIdx].getType();
}

uint32_t ThirteenPeerCard::getWeight() {
	return m_vGroups[m_nCurGroupIdx].getWeight();
}

uint32_t ThirteenPeerCard::getWeight(uint8_t nDao) {
	if (nDao < DAO_MAX) {
		return m_vGroups[nDao].getWeight();
	}
	return 0;
}

uint8_t ThirteenPeerCard::getType(uint8_t nDao) {
	if (nDao < DAO_MAX) {
		return m_vGroups[nDao].getType();
	}
	return 0;
}

void ThirteenPeerCard::reset()
{
	m_nCurGroupIdx = DAO_HEAD;
	for (auto& group : m_vGroups) {
		group.reset();
	}
	m_vHoldCards.clear();
}

PK_RESULT ThirteenPeerCard::pk(IPeerCard* pTarget) {
	assert(pTarget && "pk target is null");
	auto nType = getType();
	auto nTargetType = ((ThirteenPeerCard*)pTarget)->getType();
	if (nType == nTargetType) {
		auto nWeight = getWeight();
		auto nTargetWeight = pTarget->getWeight();
		if (nWeight > nTargetWeight) {
			return PK_RESULT_WIN;
		}
		else if (nWeight < nTargetWeight) {
			return PK_RESULT_FAILED;
		}
		return PK_RESULT_EQUAL;
	}
	else {
		if (nType > nTargetType)
		{
			return PK_RESULT_WIN;
		}
		return PK_RESULT_FAILED;
	}
}

void ThirteenPeerCard::setCurGroupIdx(uint8_t nGrpIdx)
{
	if (nGrpIdx < DAO_MAX)
	{
		m_nCurGroupIdx = nGrpIdx;
	}
	else {
		LOGFMTE("why you set a invalid group idx = %u", nGrpIdx);
	}
}

bool ThirteenPeerCard::getGroupInfo(uint8_t nGroupIdx, uint8_t& nGroupType, std::vector<uint8_t>& vGroupCards) {
	if (nGroupIdx > DAO_TAIL)
	{
		LOGFMTE("invalid group idx = %u, can not get info", nGroupIdx);
		return false;
	}

	auto& pGInfo = m_vGroups[nGroupIdx];
	if (pGInfo.getCardCnt()) {
		nGroupType = pGInfo.getType();
		for (uint8_t nIdx = 0; nIdx < pGInfo.getCardCnt() && pGInfo.getWeight() > 0; ++nIdx)
		{
			vGroupCards.push_back(pGInfo.getCardByIdx(nIdx));
		}
	}
	
	return true;
}

bool ThirteenPeerCard::holdCardToJson(Json::Value& vHoldCards)
{
	for (auto& ref : m_vHoldCards)
	{
		vHoldCards[vHoldCards.size()] = ref;
	}
	return true;
}

bool ThirteenPeerCard::groupCardToJson(Json::Value& vHoldCards)
{
	for (auto& ref : m_vGroups)
	{
		if (ref.getWeight() == 0)
		{
			continue;
		}
		//Json::Value jsPeerGroup;
		ref.holdCardToJson(vHoldCards);
		//vHoldCards[vHoldCards.size()] = jsPeerGroup;
	}
	return true;
}

bool ThirteenPeerCard::groupCardTypeToJson(Json::Value& js) {
	for (auto& ref : m_vGroups) {
		if (ref.getWeight() == 0)
		{
			continue;
		}
		js[js.size()] = ref.getType();
	}
	return true;
}

bool ThirteenPeerCard::groupCardWeightToJson(Json::Value& js) {
	for (auto& ref : m_vGroups) {
		if (ref.getWeight() == 0)
		{
			continue;
		}
		js[js.size()] = ref.getWeight();
	}
	return true;
}

void ThirteenPeerCard::getHoldCards(std::vector<uint8_t>& vHoldCards)
{
	vHoldCards.assign(m_vHoldCards.begin(), m_vHoldCards.end());
}

ThirteenPeerCard::stGroupCard::stGroupCard()
{
	reset();
}

void ThirteenPeerCard::stGroupCard::reset()
{
	m_vCard.clear();
	m_eCardType = Thirteen_None;
	m_nWeight = 0;
}

bool ThirteenPeerCard::stGroupCard::setCard(VEC_CARD& vCompsitCard)
{
	m_vCard.clear();
	for (auto ref : vCompsitCard) {
		addCardToVecAsc(m_vCard, ref);
	}
	caculateCards();
	return true;
}

bool ThirteenPeerCard::stGroupCard::holdCardToJson(Json::Value& js) {
	for (auto& ref : m_vCard) {
		js[js.size()] = ref;
	}
	return true;
}

uint8_t ThirteenPeerCard::stGroupCard::getCardByIdx(uint8_t nIdx)
{
	if (m_vCard.size() <= nIdx)
	{
		LOGFMTE("get card from group but invalid nidx = %u", nIdx);
		return 0;
	}
	return m_vCard[nIdx];
}

ThirteenType ThirteenPeerCard::stGroupCard::getType() {
	if (isCaculated() == false) {
		caculateCards();
	}
	return m_eCardType;
}

uint32_t ThirteenPeerCard::stGroupCard::getWeight() {
	if (isCaculated() == false) {
		caculateCards();
	}
	return m_nWeight;
}

bool ThirteenPeerCard::stGroupCard::isCaculated() {
	return m_eCardType > Thirteen_None;
}

void ThirteenPeerCard::stGroupCard::caculateCards() {
	ThirteenCardTypeChecker::getInstance()->checkCardType(m_vCard, m_nWeight, m_eCardType);
}

bool ThirteenPeerCard::autoSetDao(VEC_CARD& vCards, uint8_t nIdx) {
	if (nIdx < DAO_MAX) {
		if (nIdx > DAO_HEAD) {
			if (vCards.size() > OTHER_DAO_CARD_COUNT) {
				if (set5Tong(vCards, nIdx)) {
					return true;
				}
				else if (setStraightFlush(vCards, nIdx)) {
					return true;
				}
				else if (setTieZhi(vCards, nIdx)) {
					return true;
				}
				else if (setFuLu(vCards, nIdx)) {
					return true;
				}
				else if (setFlush(vCards, nIdx)) {
					return true;
				}
				else if (setStraight(vCards, nIdx)) {
					return true;
				}
				else if (setThreeCards(vCards, nIdx)) {
					return true;
				}
				else if (setDoubleDouble(vCards, nIdx)) {
					return true;
				}
				else if (setDouble(vCards, nIdx)) {
					return true;
				}
				else if (setSingle(vCards, nIdx)) {
					return true;
				}
			}
		}
		else {
			if (vCards.size() == HEAD_DAO_CARD_COUNT) {
				return m_vGroups[DAO_HEAD].setCard(vCards);
			}
		}
	}
	return false;
}

bool ThirteenPeerCard::set5Tong(VEC_CARD& vCards, uint8_t nIdx) {
	if (vCards.size() < OTHER_DAO_CARD_COUNT) {
		return false;
	}
	uint8_t nJokerCnt = 0;
	VEC_CARD vTemp;
	for (auto ref : vCards) {
		auto refType = TT_PARSE_TYPE(ref);
		if (refType == ePoker_Joker) {
			nJokerCnt++;
		}
		else {
			vTemp.push_back(ref);
		}
	}
	uint8_t nSameCnt = 0;
	uint8_t nNeedJokerCnt = 0;
	uint8_t nSameValue = 0;
	for (uint8_t i = 0; i < vTemp.size(); i++) {
		nSameCnt++;
		auto tValue = TT_PARSE_VALUE(vTemp[i]);
		if (i + 1 < vTemp.size() && tValue == TT_PARSE_VALUE(vTemp[i + 1])) {
			continue;
		}
		if (nSameCnt + nJokerCnt > 4) {
			nNeedJokerCnt = nSameCnt < 5 ? 5 - nSameCnt : 0;
			nSameValue = tValue;
			break;
		}
		nSameCnt = 0;
	}
	if (nSameValue) {
		uint8_t nNeedCnt = OTHER_DAO_CARD_COUNT;
		uint8_t nNeedOtherCnt = OTHER_DAO_CARD_COUNT - 5;
		vTemp.clear();
		for (auto ref : vCards) {
			if (nNeedCnt) {
				auto refType = TT_PARSE_TYPE(ref);
				if (refType == ePoker_Joker) {
					if (nNeedJokerCnt) {
						vTemp.push_back(ref);
						nNeedJokerCnt--;
						nNeedCnt--;
					}
					continue;
				}
				auto refValue = TT_PARSE_VALUE(ref);
				if (refValue == nSameValue) {
					vTemp.push_back(ref);
					nNeedCnt--;
				}
				else {
					if (nNeedOtherCnt) {
						vTemp.push_back(ref);
						nNeedCnt--;
						nNeedOtherCnt--;
					}
				}
			}
			else {
				break;
			}
		}
		if (vTemp.size() == OTHER_DAO_CARD_COUNT) {
			if (m_vGroups[nIdx].setCard(vTemp)) {
				for (auto ref : vTemp) {
					eraseVector(ref, vCards);
				}
				return true;
			}
		}
	}
	return false;
}

bool ThirteenPeerCard::setStraightFlush(VEC_CARD& vCards, uint8_t nIdx) {
	if (vCards.size() < OTHER_DAO_CARD_COUNT) {
		return false;
	}
	VEC_CARD vHuaCards[ePoker_NoJoker];
	VEC_CARD vJoker;
	uint8_t nJokerCnt = 0;
	for (auto ref : vCards) {
		auto refType = TT_PARSE_TYPE(ref);
		if (refType == ePoker_Joker) {
			vJoker.push_back(ref);
			nJokerCnt++;
			continue;
		}
		else if (refType < ePoker_NoJoker) {
			vHuaCards[refType].push_back(ref);
		}
	}
	VEC_CARD vTemp;
	bool is12345 = false;
	uint8_t bigValue = 0;
	uint8_t needJokerCnt = 0;
	for (auto& ref : vHuaCards) {
		if (ref.size() + nJokerCnt < OTHER_DAO_CARD_COUNT) {
			continue;
		}
		VEC_CARD vTemp_1;
		uint8_t needJokerCnt_1 = 0;
		uint8_t bigValue_1 = 0;
		if (setCommonStraight(ref, vTemp_1, nJokerCnt, needJokerCnt_1, bigValue_1)) {
			if (vTemp_1.empty()) {
				continue;
			}
			if (bigValue_1 == 14) {
				bigValue = bigValue_1;
				needJokerCnt = needJokerCnt_1;
				is12345 = false;
				vTemp.clear();
				vTemp.assign(vTemp_1.begin(), vTemp_1.end());
			}
			else {
				if (is12345 || bigValue < 14) {
					VEC_CARD vTemp_2;
					uint8_t needJokerCnt_2 = 0;
					if (set12345(ref, vTemp_2, nJokerCnt, needJokerCnt_2)) {
						if (vTemp_2.empty()) {
							continue;
						}
						vTemp.clear();
						vTemp.assign(vTemp_2.begin(), vTemp_2.end());
						is12345 = true;
						needJokerCnt = needJokerCnt_2;
						continue;
					}
				}

				if (bigValue > bigValue_1) {
					continue;
				}

				is12345 = false;
				needJokerCnt = needJokerCnt_1;
				bigValue = bigValue_1;
				vTemp.clear();
				vTemp.assign(vTemp_1.begin(), vTemp_1.end());
			}
		}
		else if ((is12345 || bigValue < 14) && set12345(ref, vTemp_1, nJokerCnt, needJokerCnt_1)) {
			vTemp.clear();
			vTemp.assign(vTemp_1.begin(), vTemp_1.end());
			is12345 = true;
			needJokerCnt = needJokerCnt_1;
		}
	}

	if (vTemp.empty()) {
		return false;
	}

	for (auto ref : vTemp) {
		eraseVector(ref, vCards);
	}
	if (eraseJoker(vCards, needJokerCnt)) {
		for (uint8_t i = 0; i < vJoker.size(); i++) {
			addCardToVecAsc(vTemp, vJoker[i]);
		}
	}
	m_vGroups[nIdx].setCard(vTemp);
	return true;
}

bool ThirteenPeerCard::setTieZhi(VEC_CARD& vCards, uint8_t nIdx) {
	if (vCards.size() < OTHER_DAO_CARD_COUNT) {
		return false;
	}
	uint8_t nJokerCnt = 0;
	VEC_CARD vTemp;
	for (auto ref : vCards) {
		auto refType = TT_PARSE_TYPE(ref);
		if (refType == ePoker_Joker) {
			nJokerCnt++;
		}
		else {
			vTemp.push_back(ref);
		}
	}
	uint8_t nSameCnt = 0;
	uint8_t nNeedJokerCnt = 0;
	uint8_t nSameValue = 0;
	for (uint8_t i = 0; i < vTemp.size(); i++) {
		nSameCnt++;
		auto tValue = TT_PARSE_VALUE(vTemp[i]);
		if (i + 1 < vTemp.size() && tValue == TT_PARSE_VALUE(vTemp[i + 1])) {
			continue;
		}
		if (nSameCnt + nJokerCnt > 3) {
			nNeedJokerCnt = nSameCnt < 4 ? 4 - nSameCnt : 0;
			nSameValue = tValue;
			break;
		}
		nSameCnt = 0;
	}
	if (nSameValue) {
		uint8_t nNeedCnt = OTHER_DAO_CARD_COUNT;
		uint8_t nNeedOtherCnt = OTHER_DAO_CARD_COUNT - 4;
		vTemp.clear();
		for (auto ref : vCards) {
			if (nNeedCnt) {
				auto refType = TT_PARSE_TYPE(ref);
				if (refType == ePoker_Joker) {
					if (nNeedJokerCnt) {
						vTemp.push_back(ref);
						nNeedJokerCnt--;
						nNeedCnt--;
					}
					continue;
				}
				auto refValue = TT_PARSE_VALUE(ref);
				if (refValue == nSameValue) {
					vTemp.push_back(ref);
					nNeedCnt--;
				}
				else {
					if (nNeedOtherCnt) {
						vTemp.push_back(ref);
						nNeedCnt--;
						nNeedOtherCnt--;
					}
				}
			}
			else {
				break;
			}
		}
		if (vTemp.size() == OTHER_DAO_CARD_COUNT) {
			if (m_vGroups[nIdx].setCard(vTemp)) {
				for (auto ref : vTemp) {
					eraseVector(ref, vCards);
				}
				return true;
			}
		}
	}
	return false;
}

bool ThirteenPeerCard::setFuLu(VEC_CARD& vCards, uint8_t nIdx) {
	if (vCards.size() < OTHER_DAO_CARD_COUNT) {
		return false;
	}

	uint8_t nJokerCnt = 0;
	VEC_CARD vTemp;
	for (auto ref : vCards) {
		auto refType = TT_PARSE_TYPE(ref);
		if (refType == ePoker_Joker) {
			nJokerCnt++;
		}
		else {
			vTemp.push_back(ref);
		}
	}
	if (nJokerCnt > 1) {
		return false;
	}
	uint8_t nSame3 = 0;
	uint8_t nSame2 = 0;
	VEC_CARD vSame2;
	uint8_t nBigValue = 0;
	uint8_t nSameCnt = 0;
	for (uint8_t i = 0; i < vTemp.size(); i++) {
		nSameCnt++;
		auto tValue = TT_PARSE_VALUE(vTemp[i]);
		if (i + 1 < vTemp.size() && tValue == TT_PARSE_VALUE(vTemp[i + 1])) {
			continue;
		}
		if (nSameCnt == 3) {
			if (nSame3) {
				//continue;
			}
			else {
				nSame3 = tValue;
			}
		}
		else if (nSameCnt == 2) {
			vSame2.push_back(tValue);
		}
		else if (nSameCnt == 1) {
			if (nBigValue) {
				//continue;
			}
			else {
				nBigValue = tValue;
			}
		}
		nSameCnt = 0;
	}

	uint8_t nNeedJokerCnt = 0;
	if (nSame3) {
		if (vSame2.empty()) {
			if (nJokerCnt > 0 && nBigValue) {
				nNeedJokerCnt = 1;
				nSame2 = nBigValue;
			}
			else {
				return false;
			}
		}
		else {
			nSame2 = vSame2[0];
		}
	}
	else {
		if (vSame2.size() > 2 && nJokerCnt > 0) {
			nSame3 = vSame2[0];
			nSame2 = vSame2[1];
			nNeedJokerCnt = 1;
		}
		else {
			return false;
		}
	}

	uint8_t nSame3Cnt = 3;
	uint8_t nSame2Cnt = 2;
	if (nSame3 && nSame2) {
		uint8_t nNeedOtherCard = 5 - OTHER_DAO_CARD_COUNT;
		vTemp.clear();
		for (auto ref : vCards) {
			if (vTemp.size() >= OTHER_DAO_CARD_COUNT) {
				break;
			}
			auto refType = TT_PARSE_TYPE(ref);
			if (refType == ePoker_Joker) {
				if (nNeedJokerCnt) {
					nNeedJokerCnt--;
					vTemp.push_back(ref);
				}
				continue;
			}
			auto refValue = TT_PARSE_VALUE(ref);
			if (refValue == nSame2 && nSame2Cnt) {
				vTemp.push_back(ref);
				nSame2Cnt--;
				continue;
			}
			if (refValue == nSame3 && nSame3Cnt) {
				vTemp.push_back(ref);
				nSame3Cnt--;
				continue;
			}
			if (nNeedOtherCard) {
				vTemp.push_back(ref);
				nNeedOtherCard--;
				continue;
			}
		}

		if (vTemp.size() == OTHER_DAO_CARD_COUNT) {
			if (m_vGroups[nIdx].setCard(vTemp)) {
				for (auto ref : vTemp) {
					eraseVector(ref, vCards);
				}
				return true;
			}
		}
	}
	return false;
}

bool ThirteenPeerCard::setFlush(VEC_CARD& vCards, uint8_t nIdx) {
	if (vCards.size() < OTHER_DAO_CARD_COUNT) {
		return false;
	}

	VEC_CARD vHuaCards[ePoker_NoJoker];
	uint8_t nJokerCnt = 0;
	for (auto ref : vCards) {
		auto refType = TT_PARSE_TYPE(ref);
		if (refType == ePoker_Joker) {
			nJokerCnt++;
			continue;
		}
		else if (refType < ePoker_NoJoker) {
			vHuaCards[refType].push_back(ref);
		}
	}
	if (nJokerCnt > 2) {
		return false;
	}

	VEC_CARD vTemp;
	uint32_t nWeight = 0;
	uint8_t nNeedJokerCnt = 0;
	for (auto& ref : vHuaCards) {
		if (ref.empty()) {
			continue;
		}
		if (ref.size() + nJokerCnt < OTHER_DAO_CARD_COUNT) {
			continue;
		}
		VEC_CARD vTemp_1;
		uint32_t nWeight_1 = 0;
		uint8_t nNeedValue = 15;
		uint8_t nNeedJokerCnt_1 = 0;
		for (auto tCard : ref) {
			if (vTemp_1.size() + nNeedJokerCnt_1 >= OTHER_DAO_CARD_COUNT) {
				break;
			}
			nNeedValue--;
			auto tValue = TT_PARSE_VALUE(tCard);
			if (tValue < nNeedValue) {
				while (nNeedJokerCnt_1 < nJokerCnt) {
					if (tValue >= nNeedValue) {
						break;
					}
					nNeedJokerCnt_1++;
					nWeight_1 = (nWeight_1 << 4) | nNeedValue;
					if (vTemp_1.size() + nNeedJokerCnt_1 >= OTHER_DAO_CARD_COUNT) {
						break;
					}
					nNeedValue--;
				}
				if (vTemp_1.size() + nNeedJokerCnt_1 >= OTHER_DAO_CARD_COUNT) {
					break;
				}
			}
			vTemp_1.push_back(tCard);
			nWeight_1 = (nWeight_1 << 4) | tValue;
			nNeedValue = tValue;
		}

		if (vTemp_1.size() + nNeedJokerCnt_1 == OTHER_DAO_CARD_COUNT) {
			if (nWeight_1 >= nWeight) {
				vTemp.clear();
				vTemp.assign(vTemp_1.begin(), vTemp_1.end());
				nNeedJokerCnt = nNeedJokerCnt_1;
				nWeight = nWeight_1;
			}
		}
	}

	if (vTemp.empty()) {
		return false;
	}

	if (nNeedJokerCnt) {
		if (nNeedJokerCnt > nJokerCnt) {
			return false;
		}
		for (auto ref : vCards) {
			if (nNeedJokerCnt) {
				auto refType = TT_PARSE_TYPE(ref);
				if (refType == ePoker_Joker) {
					addCardToVecAsc(vTemp, ref);
					nNeedJokerCnt--;
				}
			}
			else {
				break;
			}
		}
	}

	if (vTemp.size() == OTHER_DAO_CARD_COUNT) {
		if (m_vGroups[nIdx].setCard(vTemp)) {
			for (auto ref : vTemp) {
				eraseVector(ref, vCards);
			}
			return true;
		}
	}
	return false;
}

bool ThirteenPeerCard::setStraight(VEC_CARD& vCards, uint8_t nIdx) {
	if (vCards.size() < OTHER_DAO_CARD_COUNT) {
		return false;
	}

	VEC_CARD vTemp;
	uint8_t nJokerCnt = 0;
	for (auto ref : vCards) {
		auto refType = TT_PARSE_TYPE(ref);
		if (refType == ePoker_Joker) {
			nJokerCnt++;
			continue;
		}
		vTemp.push_back(ref);
	}
	if (nJokerCnt > 2) {
		return false;
	}

	uint8_t bigValue = 0;
	uint8_t nNeedJokerCnt = 0;

	VEC_CARD vTemp_1;
	uint8_t needJokerCnt_1 = 0;
	if (setCommonStraight(vTemp, vTemp_1, nJokerCnt, needJokerCnt_1, bigValue)) {
		if (vTemp_1.empty()) {
			return false;
		}
		nNeedJokerCnt = needJokerCnt_1;
		VEC_CARD vTemp_2;
		if (bigValue < 14 && set12345(vTemp, vTemp_2, nJokerCnt, needJokerCnt_1)) {
			if (vTemp_2.size()) {
				vTemp_1.clear();
				vTemp_1.assign(vTemp_2.begin(), vTemp_2.end());
				nNeedJokerCnt = needJokerCnt_1;
			}
		}

		vTemp.clear();
		vTemp.assign(vTemp_1.begin(), vTemp_1.end());
	}
	else if (set12345(vTemp, vTemp_1, nJokerCnt, needJokerCnt_1)) {
		vTemp.clear();
		vTemp.assign(vTemp_1.begin(), vTemp_1.end());
		nNeedJokerCnt = needJokerCnt_1;
	}

	if (vTemp.empty()) {
		return false;
	}

	if (nNeedJokerCnt) {
		if (nNeedJokerCnt > nJokerCnt) {
			return false;
		}
		for (auto ref : vCards) {
			if (nNeedJokerCnt) {
				auto refType = TT_PARSE_TYPE(ref);
				if (refType == ePoker_Joker) {
					addCardToVecAsc(vTemp, ref);
					nNeedJokerCnt--;
				}
			}
			else {
				break;
			}
		}
	}

	if (vTemp.size() == OTHER_DAO_CARD_COUNT) {
		if (m_vGroups[nIdx].setCard(vTemp)) {
			for (auto ref : vTemp) {
				eraseVector(ref, vCards);
			}
			return true;
		}
	}
	return false;
}

bool ThirteenPeerCard::setThreeCards(VEC_CARD& vCards, uint8_t nIdx) {
	uint8_t nNeedCnt = nIdx == DAO_HEAD ? HEAD_DAO_CARD_COUNT : OTHER_DAO_CARD_COUNT;
	if (vCards.size() < nNeedCnt) {
		return false;
	}

	uint8_t nJokerCnt = 0;
	VEC_CARD vTemp;
	for (auto ref : vCards) {
		auto refType = TT_PARSE_TYPE(ref);
		if (refType == ePoker_Joker) {
			nJokerCnt++;
		}
		else {
			vTemp.push_back(ref);
		}
	}
	if (nJokerCnt > 2) {
		return false;
	}

	uint8_t nSame3 = 0;
	uint8_t nSame2 = 0;
	uint8_t nBigValue = 0;
	uint8_t nSameCnt = 0;
	uint8_t nNeedJokerCnt = 0;
	for (uint8_t i = 0; i < vTemp.size(); i++) {
		if (nSame3) {
			break;
		}
		nSameCnt++;
		auto tValue = TT_PARSE_VALUE(vTemp[i]);
		if (i + 1 < vTemp.size() && tValue == TT_PARSE_VALUE(vTemp[i + 1])) {
			continue;
		}
		if (nSameCnt == 3) {
			if (nSame3) {
				//continue;
			}
			else {
				nSame3 = tValue;
			}
		}
		else if (nSameCnt == 2) {
			if (nSame2) {
				//continue;
			}
			else {
				nSame2 = tValue;
			}
		}
		else if (nSameCnt == 1) {
			if (nBigValue) {
				//continue;
			}
			else {
				nBigValue = tValue;
			}
		}
		nSameCnt = 0;
	}

	if (nSame3) {
		if (nJokerCnt > 0) {
			return false;
		}
		nNeedJokerCnt = 0;
	}
	else if (nSame2) {
		if (nJokerCnt == 1) {
			nSame3 = nSame2;
			nNeedJokerCnt = 1;
		}
		else {
			return false;
		}
	}
	else if (nBigValue) {
		if (nJokerCnt == 2) {
			nSame3 = nJokerCnt;
			nNeedJokerCnt = 2;
		}
		else {
			return false;
		}
	}
	
	if (nSame3) {
		vTemp.clear();
		uint8_t nCurNeedCnt = nNeedCnt;
		uint8_t nNeedOtherCnt = nNeedCnt - 3;
		for (auto ref : vCards) {
			if (nCurNeedCnt == 0) {
				break;
			}
			if (TT_PARSE_TYPE(ref) == ePoker_Joker) {
				if (nNeedJokerCnt) {
					nNeedJokerCnt--;
					nCurNeedCnt--;
					vTemp.push_back(ref);
				}
				continue;
			}
			if (TT_PARSE_VALUE(ref) == nSame3) {
				nCurNeedCnt--;
				vTemp.push_back(ref);
				continue;
			}
			if (nNeedOtherCnt) {
				nCurNeedCnt--;
				nNeedOtherCnt--;
				vTemp.push_back(ref);
				continue;
			}
		}

		if (vTemp.size() == nNeedCnt) {
			if (m_vGroups[nIdx].setCard(vTemp)) {
				for (auto ref : vTemp) {
					eraseVector(ref, vCards);
				}
				return true;
			}
		}
	}
	return false;
}

bool ThirteenPeerCard::setDoubleDouble(VEC_CARD& vCards, uint8_t nIdx) {
	if (vCards.size() < OTHER_DAO_CARD_COUNT) {
		return false;
	}

	VEC_CARD vTemp;
	uint8_t nSameCnt = 0;
	uint8_t nNeedOtherCnt = OTHER_DAO_CARD_COUNT - 4;
	uint8_t nNeedCnt = OTHER_DAO_CARD_COUNT;
	uint8_t nNeedDouble = 2;
	for (uint8_t i = 0; i < vCards.size(); i++) {
		if (TT_PARSE_TYPE(vCards[i]) == ePoker_Joker) {
			return false;
		}
		if (nNeedCnt == 0) {
			break;
		}
		nSameCnt++;
		auto tValue = TT_PARSE_VALUE(vCards[i]);
		if (i + 1 < vCards.size() && TT_PARSE_VALUE(vCards[i + 1]) == tValue) {
			continue;
		}
		if (nSameCnt == 1) {
			if (nNeedOtherCnt) {
				vTemp.push_back(vCards[i]);
				nNeedOtherCnt--;
				nNeedCnt--;
			}
			nSameCnt = 0;
		}
		else if (nSameCnt == 2) {
			if (nNeedDouble) {
				vTemp.push_back(vCards[i]);
				vTemp.push_back(vCards[i - 1]);
				nNeedCnt -= 2;
				nNeedDouble--;
			}
			else if (nNeedOtherCnt) {
				vTemp.push_back(vCards[i - 1]);
				nNeedOtherCnt--;
				nNeedCnt--;
			}
			nSameCnt = 0;
		}
		else {
			return false;
		}
	}

	if (vTemp.size() == OTHER_DAO_CARD_COUNT) {
		if (m_vGroups[nIdx].setCard(vTemp)) {
			for (auto ref : vTemp) {
				eraseVector(ref, vCards);
			}
			return true;
		}
	}
	return false;
}

bool ThirteenPeerCard::setDouble(VEC_CARD& vCards, uint8_t nIdx) {
	uint8_t nNeedCnt = nIdx == DAO_HEAD ? HEAD_DAO_CARD_COUNT : OTHER_DAO_CARD_COUNT;
	if (vCards.size() < nNeedCnt) {
		return false;
	}

	uint8_t nJokerCnt = 0;
	VEC_CARD vTemp;
	uint8_t nSameCnt = 0;
	uint8_t nSame2 = 0;
	uint8_t nCurNeedCnt = nNeedCnt;
	uint8_t nNeedOtherCnt = nNeedCnt - 2;
	for (uint8_t i = 0; i < vCards.size(); i++) {
		if (nJokerCnt > 1) {
			return false;
		}
		if (nCurNeedCnt == 0) {
			break;
		}
		if (TT_PARSE_TYPE(vCards[i]) == ePoker_Joker) {
			nJokerCnt++;
			vTemp.push_back(vCards[i]);
			nCurNeedCnt--;
			continue;
		}
		nSameCnt++;
		auto tValue = TT_PARSE_VALUE(vCards[i]);
		if (i + 1 < vCards.size() && tValue == TT_PARSE_VALUE(vCards[i + 1])) {
			if (nJokerCnt) {
				return false;
			}
			//nSameCnt = 0;
			continue;
		}
		if (nSameCnt == 1) {
			if (nSame2 == 0 && nJokerCnt) {
				nSame2 = tValue;
				vTemp.push_back(vCards[i]);
				nCurNeedCnt--;
			}
			else if (nNeedOtherCnt) {
				vTemp.push_back(vCards[i]);
				nCurNeedCnt--;
				nNeedOtherCnt--;
			}
			nSameCnt = 0;
		}
		else if (nSameCnt == 2) {
			if (nSame2) {
				return false;
			}
			if (nCurNeedCnt < 2) {
				return false;
			}
			nSame2 = tValue;
			vTemp.push_back(vCards[i]);
			vTemp.push_back(vCards[i - 1]);
			nCurNeedCnt -= 2;
			nSameCnt = 0;
		}
		else {
			return false;
		}
	}
	
	if (nSame2 && vTemp.size() == nNeedCnt) {
		if (m_vGroups[nIdx].setCard(vTemp)) {
			for (auto ref : vTemp) {
				eraseVector(ref, vCards);
			}
			return true;
		}
	}
	return false;
}

bool ThirteenPeerCard::setSingle(VEC_CARD& vCards, uint8_t nIdx) {
	uint8_t nNeedCnt = nIdx == DAO_HEAD ? HEAD_DAO_CARD_COUNT : OTHER_DAO_CARD_COUNT;
	if (vCards.size() < nNeedCnt) {
		return false;
	}

	VEC_CARD vTemp;
	uint8_t nCurNeedCnt = nNeedCnt;
	for (uint8_t i = 0; i < vCards.size(); i++) {
		if (nCurNeedCnt == 0) {
			break;
		}

		if (TT_PARSE_TYPE(vCards[i]) == ePoker_Joker) {
			return false;
		}

		auto tValue = TT_PARSE_VALUE(vCards[i]);
		if (i + 1 < vCards.size() && tValue == TT_PARSE_VALUE(vCards[i + 1])) {
			return false;
		}

		if (nCurNeedCnt) {
			vTemp.push_back(vCards[i]);
			nCurNeedCnt--;
		}
	}

	if (vTemp.size() == nNeedCnt) {
		if (m_vGroups[nIdx].setCard(vTemp)) {
			for (auto ref : vTemp) {
				eraseVector(ref, vCards);
			}
			return true;
		}
	}
	return false;
}

bool ThirteenPeerCard::setCommonStraight(VEC_CARD vCards, VEC_CARD& vTemp, uint8_t nJokerCnt, uint8_t& needJokerCnt, uint8_t& nBigValue) {
	if (vCards.size() < OTHER_DAO_CARD_COUNT) {
		return false;
	}
	needJokerCnt = 0;
	if (nJokerCnt >= OTHER_DAO_CARD_COUNT - 1) {
		needJokerCnt = OTHER_DAO_CARD_COUNT - 1;
		vTemp.push_back(vCards[0]);
		nBigValue = OTHER_DAO_CARD_COUNT + TT_PARSE_VALUE(vCards[0]) > 14 ? 14 : OTHER_DAO_CARD_COUNT + TT_PARSE_VALUE(vCards[0]);
		return true;
	}
	for (uint8_t i = 0; i < vCards.size(); i++) {
		if (vTemp.size() + nJokerCnt >= OTHER_DAO_CARD_COUNT) {
			auto tValue = TT_PARSE_VALUE(vTemp[0]);
			if (vTemp.size() + needJokerCnt < OTHER_DAO_CARD_COUNT) {
				uint8_t extraJokerCnt = OTHER_DAO_CARD_COUNT - vTemp.size() - needJokerCnt;
				if (tValue + extraJokerCnt > 14) {
					nBigValue = 14;
					needJokerCnt += 14 - tValue;
					extraJokerCnt -= 14 - tValue;
					for (uint8_t i = 1; i <= extraJokerCnt; i++) {
						tValue = TT_PARSE_VALUE(vTemp[vTemp.size() - 1]) - i;
						auto tPCard = std::find_if(vTemp.begin(), vTemp.end(), [tValue](uint8_t& nCard) {
							return tValue == TT_PARSE_VALUE(nCard);
						});
						if (tPCard == vTemp.end()) {
							needJokerCnt += 1;
						}
						else {
							addCardToVecAsc(vTemp, *tPCard);
						}
					}
				}
				else {
					nBigValue = tValue + extraJokerCnt > 14 ? 14 : tValue + extraJokerCnt;
					needJokerCnt += extraJokerCnt;
				}
			}
			else {
				nBigValue = tValue;
			}
			return true;
		}
		if (i + 1 < vCards.size()) {
			auto tValue = TT_PARSE_VALUE(vCards[i]);
			auto tValue_1 = TT_PARSE_VALUE(vCards[i + 1]);
			if (tValue == tValue_1) {
				continue;
			}
			else if (tValue == tValue_1 + 1) {
				if (vTemp.empty()) {
					vTemp.push_back(vCards[i]);
				}
				vTemp.push_back(vCards[i + 1]);
			}
			else {
				if (tValue > tValue_1) {
					uint8_t nDis = tValue - tValue_1 - 1;
					if (needJokerCnt + nDis > nJokerCnt) {
						vTemp.clear();
						needJokerCnt = 0;
					}
					else {
						if (vTemp.empty()) {
							vTemp.push_back(vCards[i]);
						}
						vTemp.push_back(vCards[i + 1]);
						needJokerCnt += nDis;
					}
				}
				else {
					vTemp.clear();
					needJokerCnt = 0;
				}
			}
		}
	}
	needJokerCnt = 0;
	vTemp.clear();
	return false;
}

bool ThirteenPeerCard::set12345(VEC_CARD vCards, VEC_CARD& vTemp, uint8_t nJokerCnt, uint8_t& needJokerCnt) {
	if (vCards.size() < OTHER_DAO_CARD_COUNT) {
		return false;
	}
	needJokerCnt = 0;
	for (uint8_t i = 1; i <= OTHER_DAO_CARD_COUNT; i++) {
		auto tCardP = std::find_if(vCards.begin(), vCards.end(), [i](uint8_t& nCard) {
			return TT_PARSE_VALUE(nCard) == (i == 1 ? 14 : i);
		});
		if (tCardP == vCards.end()) {
			if (needJokerCnt < nJokerCnt) {
				needJokerCnt++;
			}
			else {
				needJokerCnt = 0;
				vTemp.clear();
				return false;
			}
		}
		else {
			addCardToVecAsc(vTemp, *tCardP);
		}
	}
	return true;
}

bool ThirteenPeerCard::eraseJoker(VEC_CARD& vCards, uint8_t nCnt) {
	if (nCnt == 0 || nCnt > 10) {
		return false;
	}
	if (std::count_if(vCards.begin(), vCards.end(), [](uint8_t& nCard) {
		return TT_PARSE_TYPE(nCard) == ePoker_Joker;
	}) < nCnt) {
		return false;
	}
	while (nCnt > 0) {
		nCnt--;
		VEC_CARD::iterator it = find_if(vCards.begin(), vCards.end(), [](uint8_t& nCard) {
			return TT_PARSE_TYPE(nCard) == ePoker_Joker;
		});
		if (it != vCards.end())
		{
			vCards.erase(it);
		}
		else {
			return false;
		}
	}
	return true;
}