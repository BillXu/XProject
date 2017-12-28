#include "GoldenPeerCard.h"
#include "log4z.h"
#include <algorithm>
CGoldenPeerCard::CGoldenPeerCard(){
    m_nAddIdx = 0;
    m_eType = Golden_None;

	m_bEnable235 = false;
	m_bEnableStraightWin = false ;

	m_vHoldCards.resize(GOLDEN_HOLD_CARD_COUNT);
}

void CGoldenPeerCard::setOpts(bool isEnable235, bool isEnableStraightWin)
{
	m_bEnable235 = isEnable235;
	m_bEnableStraightWin = isEnableStraightWin;
}

void CGoldenPeerCard::addCompositCardNum( uint8_t nCardCompositNum )
{
	assert( (m_nAddIdx < GOLDEN_HOLD_CARD_COUNT) && "too many cards" ) ;
	if ((m_nAddIdx < GOLDEN_HOLD_CARD_COUNT) == false)
	{
		return;
	}

	m_vHoldCards[m_nAddIdx].RsetCardByCompositeNum(nCardCompositNum) ;
	++m_nAddIdx ;
}

const char*  CGoldenPeerCard::getNameString()
{
	if ( ! isCaculated() )
	{
		caculateCards();
	}
	return "golden" ;
}

uint8_t CGoldenPeerCard::getType()
{
	if ( ! isCaculated() )
	{
		caculateCards();
	}
	return m_eType;
}

uint32_t CGoldenPeerCard::getWeight() {
	std::vector<CCard> vHoldCards;
	uint32_t nWeight = getType();
	if (getCaculatedCards(vHoldCards) != GOLDEN_HOLD_CARD_COUNT) {
		return 0;
	}
	/*std::sort(vHoldCards.begin(), vHoldCards.end(), [](const CCard &cCard1, const CCard &cCard2) {
		return cCard1.GetCardFaceNum() > cCard2.GetCardFaceNum();
	});*/
	for (uint8_t i = 0; i < vHoldCards.size(); i++) {
		nWeight = (nWeight << 4) | vHoldCards[i].GetCardFaceNum(true);
	}
	return nWeight;
}

void CGoldenPeerCard::reset()
{
	m_nAddIdx = 0 ;
	m_eType = Golden_None;
}

CGoldenPeerCard::PK_RESULT CGoldenPeerCard::pk(IPeerCard* pTarget) {
	assert(pTarget && "pk target is null");
	auto nType = getType();
	auto nTargetType = ((CGoldenPeerCard*)pTarget)->getType();
	if (nType == nTargetType) {
		//235 > three cards(带王玩法专用)
		/*if (m_bEnable235 && nType == Golden_ThreeCards) {
			if (check235()) {
				return PK_RESULT_WIN;
			}
			else if (((CGoldenPeerCard*)pTarget)->check235()) {
				return PK_RESULT_FAILED;
			}
		}*/

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
		//同花与顺子
		if (m_bEnableStraightWin &&
			((nType == Golden_Straight && nTargetType == Golden_Flush) ||
			(nTargetType == Golden_Straight && nType == Golden_Flush))) {
			auto nTemp = nType;
			nType = nTargetType;
			nTargetType = nTemp;
		}
		//235豹子
		else if (m_bEnable235 &&
			((nType == Golden_ThreeCards && isHaveJoker() == false && ((CGoldenPeerCard*)pTarget)->check235()) ||
			(nTargetType == Golden_ThreeCards && ((CGoldenPeerCard*)pTarget)->isHaveJoker() == false && check235())))
		{
			auto nTemp = nType;
			nType = nTargetType;
			nTargetType = nTemp;
		}

		if (nType > nTargetType)
		{
			return PK_RESULT_WIN;
		}
		return PK_RESULT_FAILED;
	}
}

void CGoldenPeerCard::toJson(Json::Value& js)
{
	std::vector<uint8_t> vHolds;
	if ( getHoldCards(vHolds) == 0 )
	{
		return;
	}

	for (auto& ref : vHolds)
	{
		js[js.size()] = ref;
	}
}

bool CGoldenPeerCard::isCaculated()
{
	return Golden_None != m_eType ;
}

void CGoldenPeerCard::caculateCards()
{
	if (m_nAddIdx != GOLDEN_HOLD_CARD_COUNT) {
		return;
	}
	caculateCards(m_eType, m_vHoldCards);
}

void CGoldenPeerCard::caculateCards( GoldenType& type, std::vector<CCard> vHoldCards )
{
	//assert(m_nAddIdx == ( NIUNIU_HOLD_CARD_COUNT ) && "cards not enough" );
	type = Golden_Single;
	if (checkThreeCards(vHoldCards))
	{
		type = Golden_ThreeCards;
	}
	else if (checkStraightFlush(vHoldCards))
	{
		type = Golden_StraightFlush;
	}
	else if (checkFlush(vHoldCards))
	{
		type = Golden_Flush;
	}
    else if ( checkStraight(vHoldCards))
    {
		type = Golden_Straight;
    }
	else if (checkDouble(vHoldCards))
	{
		type = Golden_Double;
	}
}

bool CGoldenPeerCard::checkThreeCards(std::vector<CCard> vHoldCards)
{
	uint8_t nFaceNmuber = 0;
	for (auto cCard : vHoldCards) {
		if (nFaceNmuber) {
			if (cCard.isJoker() || cCard.GetCardFaceNum() == nFaceNmuber) {
				continue;
			}
			else {
				return false;
			}
		}
		else if(cCard.isJoker() == false){
			nFaceNmuber = cCard.GetCardFaceNum();
		}
	}
	return true;
}

bool CGoldenPeerCard::checkStraightFlush(std::vector<CCard> vHoldCards)
{
	return checkStraight(vHoldCards) && checkFlush(vHoldCards);
}

bool CGoldenPeerCard::checkFlush(std::vector<CCard> vHoldCards)
{
	uint8_t nCardType = -1;
	for (auto cCard : vHoldCards) {
		if (nCardType == uint8_t(-1)) {
			if (cCard.isJoker() == false) {
				nCardType = cCard.GetType();
			}
		}
		else {
			if (cCard.isJoker() || cCard.GetType() == nCardType) {
				continue;
			}
			else {
				return false;
			}
		}
	}

	return true;
}

bool CGoldenPeerCard::checkStraight(std::vector<CCard> vHoldCards)
{
	auto checkF = [this, vHoldCards](bool specialA) -> bool {
		std::vector<CCard> vTemp;
		for (auto cCard : vHoldCards) {
			if (cCard.isJoker() == false) {
				addCardToVecAsc(vTemp, cCard, specialA);
			}
		}

		for (uint8_t i = vTemp.size(); i > 1; i--) {
			if (vTemp[i - 1].GetCardFaceNum(specialA) + 1 == vTemp[i - 2].GetCardFaceNum(specialA)) {
				continue;
			}
			return false;
		}

		return true;
	};

	return checkF(true) || checkF(false);
}

bool CGoldenPeerCard::checkDouble(std::vector<CCard> vHoldCards)
{
	for (auto cCard : vHoldCards) {
		if (cCard.isJoker()) {
			return true;
		}
		else {
			if (std::count_if(vHoldCards.begin(), vHoldCards.end(), [cCard](CCard& cCard_1) {
				return cCard.GetCardFaceNum() == cCard_1.GetCardFaceNum();
			}) > 1) {
				return true;
			}
		}
	}

	return false;
}

bool CGoldenPeerCard::check235() {
	//暂时不支持235的joker拼凑，会有造成不可预知的后果
	if (isHaveJoker()) {
		return false;
	}
	std::vector<CCard> vHoldCards;
	if (getCaculatedCards(vHoldCards) != GOLDEN_HOLD_CARD_COUNT) {
		return false;
	}
	uint8_t n235[GOLDEN_HOLD_CARD_COUNT] = { 2, 3, 5 };
	for (int i = 0; i < GOLDEN_HOLD_CARD_COUNT; i++) {
		if (vHoldCards[i].isJoker() || vHoldCards[i].GetCardFaceNum() == n235[i]) {
			continue;
		}
		else {
			return false;
		}
	}
	return true;
}

bool CGoldenPeerCard::isHaveJoker()
{
	for (auto& ref : m_vHoldCards )
	{
		if (CCard::eCard_BigJoker == ref.GetType() || CCard::eCard_Joker == ref.GetType())
		{
			return true;
		}
	}
	return false;
}



uint8_t CGoldenPeerCard::getCaculatedCards(std::vector<CCard>& vHoldCards) {
	if (!isCaculated())
	{
		caculateCards();
	}

	if (Golden_None == m_eType || Golden_Max == m_eType) {
		return 0;
	}

	vHoldCards.clear();

	std::vector<CCard> vJoker;
	for (auto cCard : m_vHoldCards) {
		if (cCard.isJoker()) {
			addCardToVecAsc(vJoker, cCard);
		}
		else {
			addCardToVecAsc(vHoldCards, cCard);
		}
	}
	if (vJoker.size()) {
		switch (m_eType)
		{
		case Golden_ThreeCards: {
			for (auto cJoker : vJoker) {
				auto nFaceNumber = vHoldCards.begin()->GetCardFaceNum();
				uint8_t nType = CCard::eCard_Sword;
				for (; nType >= CCard::eCard_Diamond; nType--) {
					if (std::find_if(vHoldCards.begin(), vHoldCards.end(), [nType](CCard& cCard) {
						return cCard.GetType() == nType;
					}) == vHoldCards.end()) {
						cJoker.SetCard((CCard::eCardType)nType, nFaceNumber);
						break;
					}
				}
				addCardToVecAsc(vHoldCards, cJoker);
			}
			break;
		}
		case Golden_StraightFlush: {
			bool is123 = false;
			auto nType = vHoldCards.begin()->GetType();
			auto nFaceNumber = vHoldCards.begin()->GetCardFaceNum();
			if (nFaceNumber == 1) {
				if (std::find_if(vHoldCards.begin(), vHoldCards.end(), [](CCard& cCard) {
					return cCard.GetCardFaceNum() == 2 || cCard.GetCardFaceNum() == 3;
				}) != vHoldCards.end()) {
					is123 = true;
					std::sort(vHoldCards.begin(), vHoldCards.end(), [](CCard& left, CCard& right) {
						if (left.GetCardFaceNum() == right.GetCardFaceNum()) {
							return left.GetType() > right.GetType();
						}
						return left.GetCardFaceNum() > right.GetCardFaceNum();
					});
					nFaceNumber = vHoldCards.begin()->GetCardFaceNum();
				}
			}
			for (auto cJoker : vJoker) {
				if (vHoldCards.size() > 1) {
					if (nFaceNumber == 1) {
						nFaceNumber = 13;
						for (; nFaceNumber > 0; nFaceNumber--) {
							if (std::find_if(vHoldCards.begin(), vHoldCards.end(), [nFaceNumber](CCard& cCard) {
								return cCard.GetCardFaceNum() == nFaceNumber;
							}) == vHoldCards.end()) {
								cJoker.SetCard(nType, nFaceNumber);
								break;
							}
						}
					}
					else {
						auto nFind = nFaceNumber;
						bool checkFlag = true;
						for (uint8_t i = 1; i < vHoldCards.size(); i++) {
							nFind--;
							if (std::find_if(vHoldCards.begin(), vHoldCards.end(), [nFind](CCard& cCard) {
								return cCard.GetCardFaceNum() == nFind;
							}) == vHoldCards.end()) {
								cJoker.SetCard(nType, nFind);
								checkFlag = false;
								break;
							}
						}
						if (checkFlag) {
							nFaceNumber++;
							cJoker.SetCard(nType, nFaceNumber == 14 ? 1 : nFaceNumber);
						}
					}
				}
				else {
					if (nFaceNumber == 1) {
						nFaceNumber = 13;
					}
					else if (nFaceNumber == 13) {
						nFaceNumber = 1;
					}
					else {
						nFaceNumber++;
					}
					cJoker.SetCard(nType, nFaceNumber);
				}
				addCardToVecAsc(vHoldCards, cJoker, is123 == false);
			}
			break;
		}
		case Golden_Flush: {
			auto nType = vHoldCards.begin()->GetType();
			auto nFaceNumber = vHoldCards.begin()->GetCardFaceNum();
			for (auto cJoker : vJoker) {
				for (uint8_t n = 14; n > 1; n--) {
					if (std::find_if(vHoldCards.begin(), vHoldCards.end(), [n](CCard& cCard) {
						return cCard.GetCardFaceNum(true) == n;
					}) == vHoldCards.end()) {
						cJoker.SetCard(nType, n);
						break;
					}
				}
				addCardToVecAsc(vHoldCards, cJoker);
			}
			break;
		}
		case Golden_Straight: {
			bool is123 = false;
			auto nFaceNumber = vHoldCards.begin()->GetCardFaceNum();
			if (nFaceNumber == 1) {
				if (std::find_if(vHoldCards.begin(), vHoldCards.end(), [](CCard& cCard) {
					return cCard.GetCardFaceNum() == 2 || cCard.GetCardFaceNum() == 3;
				}) != vHoldCards.end()) {
					is123 = true;
					std::sort(vHoldCards.begin(), vHoldCards.end(), [](CCard& left, CCard& right) {
						if (left.GetCardFaceNum() == right.GetCardFaceNum()) {
							return left.GetType() > right.GetType();
						}
						return left.GetCardFaceNum() > right.GetCardFaceNum();
					});
					nFaceNumber = vHoldCards.begin()->GetCardFaceNum();
				}
			}
			for (auto cJoker : vJoker) {
				if (vHoldCards.size() > 1) {
					if (nFaceNumber == 1) {
						nFaceNumber = 13;
						for (; nFaceNumber > 0; nFaceNumber--) {
							if (std::find_if(vHoldCards.begin(), vHoldCards.end(), [nFaceNumber](CCard& cCard) {
								return cCard.GetCardFaceNum() == nFaceNumber;
							}) == vHoldCards.end()) {
								cJoker.SetCard(CCard::eCard_Sword, nFaceNumber);
								break;
							}
						}
					}
					else {
						auto nFind = nFaceNumber;
						bool checkFlag = true;
						for (uint8_t i = 1; i < vHoldCards.size(); i++) {
							nFind--;
							if (std::find_if(vHoldCards.begin(), vHoldCards.end(), [nFind](CCard& cCard) {
								return cCard.GetCardFaceNum() == nFind;
							}) == vHoldCards.end()) {
								cJoker.SetCard(CCard::eCard_Sword, nFind);
								checkFlag = false;
								break;
							}
						}
						if (checkFlag) {
							nFaceNumber++;
							cJoker.SetCard(CCard::eCard_Sword, nFaceNumber == 14 ? 1 : nFaceNumber);
						}
					}
				}
				else {
					if (nFaceNumber == 1) {
						nFaceNumber = 13;
					}
					else if (nFaceNumber == 13) {
						nFaceNumber = 1;
					}
					else {
						nFaceNumber++;
					}
					cJoker.SetCard(CCard::eCard_Sword, nFaceNumber);
				}
				addCardToVecAsc(vHoldCards, cJoker, is123 == false);
			}
			break;
		}
		case Golden_Double: {
			auto nFaceNumber = vHoldCards.begin()->GetCardFaceNum();
			uint8_t nType = CCard::eCard_Sword;
			for (; nType >= CCard::eCard_Diamond; nType--) {
				if (std::find_if(vHoldCards.begin(), vHoldCards.end(), [nType, nFaceNumber](CCard& cCard) {
					return cCard.GetType() == nType && cCard.GetCardFaceNum() == nFaceNumber;
				}) == vHoldCards.end()) {
					vJoker[0].SetCard((CCard::eCardType)nType, nFaceNumber);
					break;
				}
			}
			addCardToVecAsc(vHoldCards, vJoker[0]);
			break;
		}
		}
	}

	if (m_eType == Golden_Double) {
		std::vector<CCard> vTemp;
		vJoker.clear();
		vJoker.insert(vJoker.end(), vHoldCards.begin(), vHoldCards.end());
		vHoldCards.clear();

		for (auto ref : vJoker) {
			if (std::count_if(vJoker.begin(), vJoker.end(), [ref](CCard& cCard) {
				return ref.GetCardFaceNum() == cCard.GetCardFaceNum();
			}) > 1) {
				addCardToVecAsc(vHoldCards, ref);
			}
			else {
				addCardToVecAsc(vTemp, ref);
			}
		}

		vHoldCards.insert(vHoldCards.end(), vTemp.begin(), vTemp.end());
	}
	else if (Golden_Straight == m_eType || Golden_StraightFlush == m_eType) {
		if (vJoker.size() < 1) {
			auto find_1 = std::find_if(vHoldCards.begin(), vHoldCards.end(), [](CCard& cCard) {
				return cCard.GetCardFaceNum() == 1;
			});
			auto find_3 = std::find_if(vHoldCards.begin(), vHoldCards.end(), [](CCard& cCard) {
				return cCard.GetCardFaceNum() == 3;
			});
			if (find_1 != vHoldCards.end() && find_3 != vHoldCards.end()) {
				std::sort(vHoldCards.begin(), vHoldCards.end(), [](CCard& a, CCard& b) {
					return a.GetCardFaceNum() > b.GetCardFaceNum();
				});
			}
		}
	}

	return vHoldCards.size();
}