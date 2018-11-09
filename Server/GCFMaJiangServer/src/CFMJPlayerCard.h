#pragma once
#include "MJPlayerCard.h"
#include <memory>
class CFMJFanxingChecker;
class CFMJPlayerCard
	:public MJPlayerCard
{
public:
	CFMJPlayerCard();
	~CFMJPlayerCard();
	void reset() override;
	bool canEatCard(uint8_t nCard, uint8_t nWithA = 0, uint8_t nWithB = 0) override { return false; }
	bool canHuWitCard(uint8_t nCard)override;
	bool isHoldCardCanHu(uint8_t& nJiang)override;
	void clearHuCnt();
	uint8_t getLastHuCnt();
	void getLastHuFanxing(std::vector<eFanxingType>& vFanxing);
	void onLouHu();

protected:
	bool check13Yao();
	void getFanXingAndFanCnt(std::vector<eFanxingType>& vHuTypes, uint8_t& nFanCnt);
	void sortFanCnt(std::vector<eFanxingType>& vHuTypes, uint8_t& nFanCnt);

protected:
	uint8_t m_nLastHuCnt = 0;
	std::vector<eFanxingType> m_vLastHuFanxing;

	uint8_t m_nLouHuCnt = 0;
	static CFMJFanxingChecker m_pFanxingChecker;
};