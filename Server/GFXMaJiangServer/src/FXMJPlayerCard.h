#pragma once
#include "MJPlayerCard.h"
class FXMJFanxingChecker;
class FXMJPlayerCard
	:public MJPlayerCard
{
public:
	void reset() override;
	bool canPengWithCard(uint8_t nCard) override;
	bool canMingGangWithCard(uint8_t nCard) override;
	bool canEatCard(uint8_t nCard, uint8_t nWithA = 0, uint8_t nWithB = 0) override;
	bool canAnGangWithCard(uint8_t nCard)override;
	bool canBuGangWithCard(uint8_t nCard)override;
	void onVisitPlayerCardInfo(Json::Value& js, bool isSelf)override;
	bool getHoldCardThatCanAnGang(VEC_CARD& vGangCards)override;
	bool getHoldCardThatCanBuGang(VEC_CARD& vGangCards)override;
	bool getHoldCardThatCanCyclone(VEC_CARD& vGangCards);
	bool canCycloneWithCard(uint8_t nCard);
	bool onCyclone(uint8_t nCard, uint8_t nGangGetCard);
	bool onDirectGang(uint8_t nCard, uint8_t nGangGetCard, uint16_t nInvokerIdx) override;
	bool onDirectGang(uint8_t nCard, uint16_t nInvokerIdx);
	bool onAnGang(uint8_t nCard, uint8_t nGangGetCard) override;
	bool onAnGang(uint8_t nCard);
	bool onBuGang(uint8_t nCard);
	bool getCycloneCard(VEC_CARD& vCycloneCard);
	bool canHuWitCard(uint8_t nCard) override;
	bool canRotDirectGang(uint8_t nCard);
	bool checkCanHuWithCard(uint8_t nCard);
	bool checkHoldCardCanHu();
	bool isHoldCardCanHu(uint8_t& nJiang) override;
	bool isTingPai() override;
	bool tingCheck(uint8_t nGangCard = 0);
	bool tingCheck(VEC_CARD& vCards);
	bool isEanble7Pair()override { return m_bEnable7Pair; }
	void signEnable7Pair() { m_bEnable7Pair = true; }
	bool isEnableOOT() { return m_bEnableOnlyOneType; }
	void signEnableOOT() { m_bEnableOnlyOneType = true; }
	bool isEnableSB1() { return m_bEnableSB1; }
	void signEnableSB1() { m_bEnableSB1 = true; }
	void signCool() { m_bCool = true; }
	bool isCool() { return m_bCool; }
	bool isHaveCards(VEC_CARD vCards);

	bool isHoldCardCanHu();
	//bool isHoldCardCanHu(VEC_CARD vTemp);
	//bool isHoldCardCanHu(VEC_CARD vTemp[eCT_Max]);
	uint8_t getHuCard() { return m_nHuCard; }
	void setHuCard(uint8_t nCard) { m_nHuCard = nCard; }

	bool isTing() override { return m_vTingCards.size(); }
	void clearTing();
	bool canHuOnlyOneCard();
	bool isJiaHu();
	bool isBianHu();
	bool isDanDiao();
	bool isPreGang() { return m_vPreGang.empty() == false; }
	void addPreGang(uint8_t nPreGangCard) { m_vPreGang.push_back(nPreGangCard); }
	void clearPreGang() { m_vPreGang.clear(); }
	uint8_t getHuLevel(uint8_t nHuCard);
	void onPlayerLouHu(uint8_t nCard);

protected:
	bool eraseVector(uint8_t p, VEC_CARD& typeVec);
	bool checkKezi();
	bool check3Men();
	bool check19();
	bool checkMenQing();
	bool checkSB1();
	uint8_t getHoldCardCnt();
	uint8_t getHoldCardCnt(uint8_t nCard);

protected:
	VEC_CARD m_vPreGang;
	uint8_t m_nHuCard;
	bool m_bCanHuOnlyOne;
	bool m_bCheckedCanHuOnlyOne;
	bool m_bEnable7Pair = false;
	bool m_bEnableOnlyOneType = false;
	bool m_bEnableSB1 = false;
	VEC_CARD m_vTingCards;
	bool m_bCool;
	uint8_t m_nLouHuLevel = 0;

};