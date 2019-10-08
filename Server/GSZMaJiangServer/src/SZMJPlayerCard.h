#pragma once
#include "MJPlayerCard.h"
class SZMJPlayerCard
	:public MJPlayerCard
{
public:
	struct stSZHuDetail
		:public stHuDetail
	{
	public:
		uint8_t getMingKe() {
			return m_nMingKe;
		}

		uint8_t getAnKe() {
			return m_nAnKe;
		}

		void addAnKe(uint8_t nCnt = 1) {
			m_nAnKe += nCnt;
		}

		void addMingKe(uint8_t nCnt = 1) {
			m_nMingKe += nCnt;
		}
	protected:
		uint8_t m_nAnKe = 0;
		uint8_t m_nMingKe = 0;
	};
public:
	void reset() override;
	bool canEatCard(uint8_t nCard, uint8_t nWith1 = 0, uint8_t nWith2 = 0) override { return false; }
	bool canHuWitCard(uint8_t nCard)override;
	bool canHuWitCard(uint8_t nCard, bool isRotGang);
	void onDoHu(uint16_t nInvokerIdx, uint8_t nHuCard, bool isInvokerHaveGangFlag)override;
	bool isHoldCardCanHu(uint8_t& nJiang) override;
	//uint8_t getHuaCard()override;
	bool isHaveCards(VEC_CARD vCards);

	bool isHoldCardCanHu(uint8_t& nJiang, bool isGangKai);
	uint8_t getHuCard() { return m_nHuCard; }
	void setHuCard(uint8_t nCard) { m_nHuCard = nCard; }

	uint8_t getHoldCardCnt();

	bool checkDaMenQing();
	bool checkXiaoMenQing();
	uint8_t getHuaCntWithoutHuTypeHuaCnt(stHuDetail* pHuDetail = nullptr);

	void setRuleMode(uint8_t nRuleMode);

protected:
	bool eraseVector(uint8_t p, VEC_CARD& typeVec);
	bool checkHunYiSe();
	bool checkQingYiSe();
	bool checkDuiDuiHu();
	bool checkQiDui();
	//bool checkHaoHuaQiDui();
	//bool checkDaDiaoChe();

	uint8_t getZiMoHuaRequire();
	uint8_t getDianPaoHuHuaRequire();

	//update by haodi 新增检查暗刻方法
	uint8_t checkAnKe(uint8_t nHuaCnt, stHuDetail* pHuDetail = nullptr);//算花使用，返回最终得到的花数，自摸请设置胡牌为0
	//uint8_t checkAnKe(uint8_t nHuCard);//单纯检测暗刻数量，返回手中拥有暗刻数量,自摸请设置胡牌为0

protected:
	uint8_t m_nHuCard;
	uint8_t m_nRuleMode = 1; //1 代表两摸三冲 2 代表三摸四冲

};