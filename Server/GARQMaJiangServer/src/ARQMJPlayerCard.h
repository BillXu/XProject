#pragma once
#include "MJPlayerCard.h"
class ARQMJPlayerCard
	:public MJPlayerCard
{
public:
	void reset() override;
	bool canPengWithCard(uint8_t nCard) override;
	bool canMingGangWithCard(uint8_t nCard) override;
	bool canEatCard(uint8_t nCard, uint8_t nWithA = 0, uint8_t nWithB = 0) override;
	bool canAnGangWithCard(uint8_t nCard)override;
	void onVisitPlayerCardInfo(Json::Value& js, bool isSelf)override;
	bool getHoldCardThatCanAnGang(VEC_CARD& vGangCards)override;
	bool getHoldCardThatCanBuGang(VEC_CARD& vGangCards)override;
	bool getHoldCardThatCanCyclone(VEC_CARD& vGangCards);
	bool canCycloneWithCard(uint8_t nCard);
	bool onCyclone(uint8_t nCard, uint8_t nGangGetCard);
	bool getCycloneCard(VEC_CARD& vCycloneCard);
	bool isHoldCardCanHu(uint8_t& nJiang) override;
	bool isTingPai() override;
	bool isEanble7Pair()override { return false; }
	bool isHaveCards(VEC_CARD vCards);

	bool isHoldCardCanHu();
	//bool isHoldCardCanHu(VEC_CARD vTemp);
	//bool isHoldCardCanHu(VEC_CARD vTemp[eCT_Max]);
	uint8_t getHuCard() { return m_nHuCard; }
	void setHuCard(uint8_t nCard) { m_nHuCard = nCard; }

	bool isJiaHu();
	bool isDanDiao();
	bool isBianHu();
	bool isHuOnly19();

	uint8_t getHoldCardCnt();

protected:
	bool eraseVector(uint8_t p, VEC_CARD& typeVec);
	bool checkKezi();
	bool check3Men();
	bool check19();
	bool canHuOnlyOneCard();

protected:
	VEC_CARD m_vCyclone;
	uint8_t m_nHuCard;
	bool m_bCanHuOnlyOne;
	bool m_bCheckedCanHuOnlyOne;

};