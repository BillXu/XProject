#pragma once
#include "MJPlayerCard.h"
class CFMJPlayerCard
	:public MJPlayerCard
{
public:
	void reset() override;
	bool canEatCard(uint8_t nCard, uint8_t nWithA = 0, uint8_t nWithB = 0) override { return false; }
	bool canHuWitCard(uint8_t nCard)override;
	bool isHoldCardCanHu(uint8_t& nJiang)override;
	void clearHuCnt();
	uint8_t getLastHuCnt() { return m_nLastHuCnt; }
	void getLastHuFanxing(std::vector<eFanxingType>& vFanxing);
	void onLouHu();

protected:
	bool check13Yao();
	void getFanXingAndFanCnt(std::vector<eFanxingType>& vHuTypes, uint8_t& nFanCnt);

protected:
	uint8_t m_nLastHuCnt = 0;
	std::vector<eFanxingType> m_vLastHuFanxing;

	uint8_t m_nLouHuCnt = 0;
};