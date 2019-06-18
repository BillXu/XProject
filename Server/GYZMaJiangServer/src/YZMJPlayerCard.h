#pragma once
#include "MJPlayerCard.h"
class YZMJFanxingChecker;
class YZMJPlayerCard
	:public MJPlayerCard
{
public:
	void reset() override;
	bool onChuCard(uint8_t nChuCard)override;
	bool onAnGang(uint8_t nCard, uint8_t nGangGetCard) override;
	bool canPengWithCard(uint8_t nCard) override;
	bool canMingGangWithCard(uint8_t nCard) override;
	bool canEatCard(uint8_t nCard, uint8_t nWithA = 0, uint8_t nWithB = 0) override { return false; }
	bool canHuWitCard(uint8_t nCard) override;
	bool isHoldCardCanHu(uint8_t& nJiang) override;
	uint8_t getBaiDaCard()override;
	bool isEanble7Pair()override;
	void onDoHu(uint16_t nInvokerIdx, uint8_t nHuCard, bool isInvokerHaveGangFlag)override;
	uint8_t getHuCard() { return m_nHuCard; }
	void setHuCard(uint8_t nCard) { m_nHuCard = nCard; }

	uint8_t getHoldCardCnt();
	uint8_t isHaveDa();

	void clearHuCnt();
	uint8_t getLastHuCnt();
	void getLastHuFanxing(std::vector<eFanxingType>& vFanxing);
	void onLouHu();

	uint8_t daFilter(VEC_CARD& nCards);
	bool vecHu7Pair(VEC_CARD vHuPai, uint8_t& eHunCnt);
	bool checkYiTiaoLong();

protected:
	bool eraseVector(uint8_t p, VEC_CARD& typeVec);

	void getFanXingAndFanCnt(std::vector<eFanxingType>& vHuTypes, uint8_t& nFanCnt);
	void sortFanCnt(std::vector<eFanxingType>& vHuTypes, uint8_t& nFanCnt);

protected:
	uint8_t m_nHuCard;
	bool m_bEnableHu;

	uint8_t m_nLastHuCnt = 0;
	std::vector<eFanxingType> m_vLastHuFanxing;

	uint8_t m_nLouHuCnt = 0;
	static YZMJFanxingChecker m_pFanxingChecker;

};