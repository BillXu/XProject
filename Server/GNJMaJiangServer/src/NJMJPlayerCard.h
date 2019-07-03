#pragma once
#include "MJPlayerCard.h"
class NJMJPlayerCard
	:public MJPlayerCard
{
public:
	void reset() override;
	bool canEatCard(uint8_t nCard, uint8_t nWith1 = 0, uint8_t nWith2 = 0) override { return false; }
	bool canHuWitCard(uint8_t nCard)override;
	void onDoHu(uint16_t nInvokerIdx, uint8_t nHuCard, bool isInvokerHaveGangFlag)override;
	bool isHoldCardCanHu(uint8_t& nJiang) override;
	//uint8_t getHuaCard()override;
	bool isHaveCards(VEC_CARD vCards);

	uint8_t getHuCard() { return m_nHuCard; }
	void setHuCard(uint8_t nCard) { m_nHuCard = nCard; }

	uint8_t getHoldCardCnt();

	uint8_t getHuaCntWithoutHuTypeHuaCnt();

	bool isSiLianFeng();
	bool isZiDaAnGang(uint8_t nCard);
	bool getNormalBaoPaiIdx(uint8_t& nBaoIdx);
	bool isKuaiZhaoHu(uint8_t nCard, uint8_t& nBaoIdx);
	bool isKuaiZhaoZiMo(uint8_t& nBaoIdx);

protected:
	bool checkDaHu();
	bool eraseVector(uint8_t p, VEC_CARD& typeVec);
	bool checkHunYiSe();
	bool checkQingYiSe();
	bool checkDuiDuiHu();
	bool checkQiDui();
	bool checkMenQing();
	bool checkWuHuaGuo();
	bool checkTianHu();
	bool checkDiHu();
	bool checkYiDuiDaoDi();

	//update by haodi 新增检查暗刻方法
	uint8_t checkAnKe(uint8_t nHuaCnt);//算花使用，返回最终得到的花数，自摸请设置胡牌为0
	//uint8_t checkAnKe(uint8_t nHuCard);//单纯检测暗刻数量，返回手中拥有暗刻数量,自摸请设置胡牌为0

protected:
	uint8_t m_nHuCard;

};