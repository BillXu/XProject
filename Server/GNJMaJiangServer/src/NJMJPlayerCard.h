#pragma once
#include "MJPlayerCard.h"
class NJMJPlayerCard
	:public MJPlayerCard
{
public:
	void reset() override;
	bool canEatCard(uint8_t nCard, uint8_t nWith1 = 0, uint8_t nWith2 = 0) override { return false; }
	bool canPengWithCard(uint8_t nCard) override;
	bool canHuWitCard(uint8_t nCard)override;
	void onDoHu(uint16_t nInvokerIdx, uint8_t nHuCard, bool isInvokerHaveGangFlag)override;
	bool isHoldCardCanHu(uint8_t& nJiang) override;
	bool onChuCard(uint8_t nChuCard)override;
	bool onBuHua(uint8_t nHuaCard, uint8_t nGetCard)override;
	//uint8_t getHuaCard()override;
	bool isHaveCards(VEC_CARD vCards);
	bool isPengCard(uint8_t nCard);

	uint8_t getHuCard() { return m_nHuCard; }
	void setHuCard(uint8_t nCard) { m_nHuCard = nCard; }
	void clearHuCard() { m_nHuCard = 0; }

	uint8_t getHoldCardCnt();

	uint8_t getHuaCntWithoutHuTypeHuaCnt(std::vector<eFanxingType>& vFanxing, bool isKuaiZhaoHu = false);

	bool isSiLianFeng();
	bool isZiDaAnGang(uint8_t nCard);
	bool getWaiBaoIdx(uint8_t& nBaoIdx, bool isZiMo = false);
	bool getNormalBaoIdx(uint8_t& nBaoIdx, bool isZiMo = false);
	bool checkKuaiZhaoHu(uint8_t& nBaoIdx, std::vector<eFanxingType>& vFanxing);
	bool checkKuaiZhaoZiMo(uint8_t& nBaoIdx, std::vector<eFanxingType>& vFanxing);
	bool checkQiDui(std::vector<eFanxingType>& vFanxing, bool isZiMo);
	//bool checkCanHuOnlyOneCard(std::vector<eFanxingType>& vFanxing);

	bool isTing() { return m_bTing; }
	void signTing() { m_bTing = true; }
	void clearTing() { m_bTing = false; }

	bool isQQDD() { return m_bQQDD; }
	void signQQDD() { m_bQQDD = true; }
	void clearQQDD() { m_bQQDD = false; }

	void clearKuaiZhao() { m_bKuaiZhaoHu = false; m_bKuaiZhaoZiMo = false; }

	void getNormalHuType(std::vector<eFanxingType>& vFanxing, bool isZiMo);

protected:
	bool isKuaiZhaoHu(uint8_t nCard, uint8_t& nBaoIdx);
	bool isKuaiZhaoZiMo(uint8_t& nBaoIdx);

	bool checkDaHu();
	bool eraseVector(uint8_t p, VEC_CARD& typeVec);
	bool checkHunYiSe();
	bool checkQingYiSe();
	bool checkDuiDuiHu();
	bool checkQiDui();
	bool checkMenQing();
	bool checkWuHuaGuo(bool isDaHu = false);
	bool checkTianHu();
	bool checkDiHu();
	bool checkYaJue();
	bool checkQuanQiuDuDiao();
	bool checkYiDuiDaoDi();

	bool isJiaHu();
	bool isDanDiao();
	bool isBianHu();
	bool isQueYi();

	//update by haodi 新增检查暗刻方法
	uint8_t checkAnKe(uint8_t& nHuaCnt);//算花使用，返回最终得到的花数，自摸请设置胡牌为0
	//uint8_t checkAnKe(uint8_t nHuCard);//单纯检测暗刻数量，返回手中拥有暗刻数量,自摸请设置胡牌为0

protected:
	uint8_t m_nHuCard;
	uint8_t m_nQQDDCard;

	bool m_bTing;
	bool m_bQQDD;

	bool m_bKuaiZhaoHu = false;
	bool m_bKuaiZhaoZiMo = false;
};