#include "FXMJPoker.h"
void FXMJPoker::init(Json::Value& jsOpt) {
	IMJPoker::init(jsOpt);

	// every card are 4 count 
	for (uint8_t nCnt = 0; nCnt < 4; ++nCnt)
	{
		// add feng
		for (uint8_t nValue = 1; nValue <= 4; ++nValue) {
			addCardToPoker(makeCardNumber(eCT_Feng, nValue));
		}

		// add jian
		for (uint8_t nValue = 1; nValue <= 3; ++nValue) {
			addCardToPoker(makeCardNumber(eCT_Jian, nValue));
		}
	}
}

uint8_t FXMJPoker::getCardByIdx(uint16_t nIdx, bool isReverse) {
	if (isReverse) {
		if (nIdx > m_vCards.size()) {
			nIdx = m_vCards.size();
		}
		nIdx = m_vCards.size() - nIdx;
	}
	if (nIdx >= m_vCards.size() || nIdx < m_nCurIdx) {
		nIdx = m_nCurIdx;
	}
	return m_vCards[nIdx];
}

void FXMJPoker::makeSpecialCard(std::vector<uint8_t>& vMakedCards) {
#ifndef _DEBUG
	return;
#endif // !_DEBUG
	//return;

	uint8_t tCard;
	//1
	tCard = makeCardNumber(eCT_Feng, 1);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Feng, 1);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Feng, 4);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Wan, 3);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Wan, 4);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Wan, 6);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Wan, 8);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tong, 1);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tong, 5);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tiao, 4);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tiao, 5);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tiao, 7);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tiao, 7);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tiao, 7);
	vMakedCards.push_back(tCard);

	//2
	tCard = makeCardNumber(eCT_Feng, 3);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Wan, 2);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Wan, 3);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Wan, 4);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Wan, 5);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Wan, 9);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Wan, 9);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tong, 2);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tong, 3);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tiao, 1);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tiao, 3);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tiao, 8);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tiao, 9);
	vMakedCards.push_back(tCard);

	//3
	tCard = makeCardNumber(eCT_Jian, 2);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Wan, 1);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Wan, 2);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Wan, 3);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Wan, 5);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Wan, 7);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Wan, 8);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tong, 2);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tong, 2);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tong, 5);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tiao, 4);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tiao, 5);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tiao, 9);
	vMakedCards.push_back(tCard);

	//4
	tCard = makeCardNumber(eCT_Feng, 1);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Feng, 4);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Jian, 2);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Wan, 1);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Wan, 6);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tong, 3);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tong, 4);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tong, 7);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tong, 7);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tong, 8);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tiao, 1);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tiao, 3);
	vMakedCards.push_back(tCard);

	tCard = makeCardNumber(eCT_Tiao, 6);
	vMakedCards.push_back(tCard);
}