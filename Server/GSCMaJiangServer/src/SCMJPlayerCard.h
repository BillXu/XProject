#pragma once
#include "MJPlayerCard.h"
class SCMJPlayerCard
	:public MJPlayerCard
{
public:
	void endDoHu(uint8_t nHuCard);
	void reset() override;
	bool canPengWithCard(uint8_t nCard) override;
	bool canMingGangWithCard(uint8_t nCard) override;
	bool canEatCard(uint8_t nCard) override { return false; }
	bool canAnGangWithCard(uint8_t nCard)override;
	bool canBuGangWithCard(uint8_t nCard)override;
	void onVisitPlayerCardInfo(Json::Value& js, bool isSelf)override;
	bool getHoldCardThatCanAnGang(VEC_CARD& vGangCards)override;
	bool getHoldCardThatCanBuGang(VEC_CARD& vGangCards)override;
	bool isHoldCardCanHu(uint8_t& nJiang) override;
	bool isTingPai() override;
	bool onExchageCards(VEC_CARD vOwn, VEC_CARD vExchage);
	void onAutoDecideExchageCards(uint8_t nAmount, VEC_CARD& vCards);
	bool isHaveCards(VEC_CARD vCards);
	bool isHuaZhu();

	bool isHoldCardCanHu();
	bool isHoldCardCanHu(VEC_CARD vTemp);
	bool isHoldCardCanHu(VEC_CARD vTemp[eCT_Max]);

	void onAutoSetMiss();
	void setMiss(eMJCardType nType) { m_nMissType = nType; }
	void clearMiss() { m_nMissType = eCT_None; }
	eMJCardType getMissType() { return m_nMissType; }
	bool isDecideMiss() { return m_nMissType == eCT_Tiao || m_nMissType == eCT_Tong || m_nMissType == eCT_Wan; }
	void getQueType(std::vector<eMJCardType>& vType);
	
	void addHuCard(uint8_t nCard) { m_vHuCards.push_back(nCard); }

protected:
	bool eraseVector(uint8_t p, VEC_CARD& typeVec);

protected:
	eMJCardType m_nMissType;
	VEC_CARD m_vHuCards;
};