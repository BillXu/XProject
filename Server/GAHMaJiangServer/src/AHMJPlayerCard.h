#pragma once
#include "MJPlayerCard.h"
#include <memory>
class AHMJFanxingChecker;
class AHMJPlayerCard
	:public MJPlayerCard
{
public:
	void reset() override;
	bool canEatCard(uint8_t nCard, uint8_t nWithA = 0, uint8_t nWithB = 0) override;
	bool canPengWithCard(uint8_t nCard) override;
	bool canMingGangWithCard(uint8_t nCard) override;
	bool canAnGangWithCard(uint8_t nCard)override;
	bool getHoldCardThatCanAnGang(VEC_CARD& vGangCards)override;
	bool canHuWitCard(uint8_t nCard)override;
	bool isHoldCardCanHu(uint8_t& nJiang)override;
	void clearHuCnt();
	uint8_t getLastHuCnt();
	void getLastHuFanxing(std::vector<eFanxingType>& vFanxing);
	void onLouHu();
	bool isEanble7Pair()override { return false; }
	bool isJiaHu();
	bool canHuOnlyOneCard();
	uint8_t getHoldCardCnt();
	uint8_t getHoldCardCnt(uint8_t nCard);
	uint8_t getHuCard() { return m_nHuCard; }
	void setHuCard(uint8_t nCard) { m_nHuCard = nCard; }
	bool checkMenQing();

protected:
	bool checkKezi();
	bool check3Men();
	bool check19();
	bool checkJianKePiao();
	bool checkDuiDui();

	void getFanXingAndFanCnt(std::vector<eFanxingType>& vHuTypes, uint8_t& nFanCnt);
	void sortFanCnt(std::vector<eFanxingType>& vHuTypes, uint8_t& nFanCnt);

protected:
	uint8_t m_nLastHuCnt = 0;
	std::vector<eFanxingType> m_vLastHuFanxing;

	uint8_t m_nLouHuCnt = 0;
	static AHMJFanxingChecker m_pFanxingChecker;

	uint8_t m_nHuCard;
};