#pragma once
#include "MJPlayerCard.h"
class SDMJPlayerCard
	:public MJPlayerCard
{
public:
	SDMJPlayerCard();
	void reset() override;
	bool canEatCard(uint8_t nCard, uint8_t nWith1 = 0, uint8_t nWith2 = 0) override { return false; }
	bool canPengWithCard(uint8_t nCard) override;
	bool canMingGangWithCard(uint8_t nCard)override;
	bool canAnGangWithCard(uint8_t nCard)override;
	bool canBuGangWithCard(uint8_t nCard)override;
	bool getHoldCardThatCanAnGang(VEC_CARD& vGangCards)override;
	bool getHoldCardThatCanBuGang(VEC_CARD& vGangCards)override;
	bool canHuWitCard(uint8_t nCard)override;
	bool canHuWitCard(uint8_t nCard, bool isRotGang);
	void onDoHu(uint16_t nInvokerIdx, uint8_t nHuCard, bool isInvokerHaveGangFlag)override;
	bool isHoldCardCanHu(uint8_t& nJiang) override;
	uint8_t getHuaCard()override;
	uint8_t getBaiDaCard()override;
	bool onChuCard(uint8_t nChuCard)override;
	bool isEanble7Pair()override;
	bool isEnableHHQD();
	bool isHaveCards(VEC_CARD vCards);

	bool isHoldCardCanHu(uint8_t& nJiang, bool isGangKai);
	uint8_t getHuCard() { return m_nHuCard; }
	void setHuCard(uint8_t nCard) { m_nHuCard = nCard; }

	uint8_t getHoldCardCnt();

	bool checkDaMenQing();
	bool checkXiaoMenQing();
	bool checkHunYiSe();
	bool checkQingYiSe();
	bool checkDuiDuiHu();
	bool checkQiDui();
	bool checkHaoHuaQiDui();
	uint8_t getHuaCntWithoutHuTypeHuaCnt(bool checkDMQ = false);

	void setRuleMode(uint8_t nRuleMode);
	bool isChuDa() { return m_bChuDa; }
	uint8_t isHaveDa();
	bool checkRaoDa(bool isZiMo, uint8_t nCard);
	void getAnKeHuaType(std::vector<eFanxingType> &vFanxing);

protected:
	bool eraseVector(uint8_t p, VEC_CARD& typeVec);

	uint8_t getZiMoHuaRequire();
	uint8_t getDianPaoHuHuaRequire();

	//update by haodi 新增检查暗刻方法
	uint8_t checkAnKe(uint8_t nHuaCnt, bool checkDMQ);//算花使用，返回最终得到的花数，自摸请设置胡牌为0
	//uint8_t checkAnKe(uint8_t nHuCard);//单纯检测暗刻数量，返回手中拥有暗刻数量,自摸请设置胡牌为0

	void removeHoldCardAllBaiDa();//此方法将移除手牌中所有的百搭牌，除非你知道你在做什么，否则慎用
	uint8_t daFilter(VEC_CARD& nCards);
	bool isHoldCardCanHuNormalNotFengJiang(uint8_t& nJiang);
	uint8_t getSDMiniQueCntMustJiang(VEC_CARD tCards, uint8_t nDaCnt);
	bool check7PairRaoDaAllShun(uint8_t nBaiDaCnt);
	bool vecHu7Pair(VEC_CARD vHuPai, uint8_t& eHunCnt);

protected:
	uint8_t m_nHuCard;
	uint8_t m_nRuleMode = 2; //1 代表两摸三冲 2 代表三摸四冲
	uint8_t m_nBaiDaCard;
	bool m_bChuDa;
	VEC_CARD m_vFengKe;

};