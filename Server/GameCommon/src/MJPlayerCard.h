#pragma once 
#include "IMJPlayerCard.h"
#include "MJDefine.h"
#include <algorithm>  
#include <set>
#include "CommonDefine.h"
class MJPlayerCard
	:public IMJPlayerCard
{
public:
	struct stInvokeActInfo
	{
		uint8_t nTargetCard = 0 ;
		VEC_CARD vWithCard;
		uint8_t nInvokerIdx = 0 ;
		eMJActType eAct = eMJAct_Max ;
	};
	typedef std::vector<stInvokeActInfo> VEC_INVOKE_ACT_INFO;
public:
	virtual uint8_t getBaiDaCard() { return 0;  }  // means no bai Da ;
	virtual bool isEanble7Pair() { return true; }
	void onDoHu(uint16_t nInvokerIdx, uint8_t nHuCard, bool isInvokerHaveGangFlag)override;
	void reset() override;
	void addDistributeCard(uint8_t nCardNum) final;
	bool onGangCardBeRobot(uint8_t nCard) final;
	bool onCardBeGangPengEat(uint8_t nCard) final;

	bool isHaveCard(uint8_t nCard) final;  // holdCard ;
	bool canMingGangWithCard(uint8_t nCard) override;
	bool canPengWithCard(uint8_t nCard) override;
	bool canEatCard(uint8_t nCard, uint8_t nWith1 = 0, uint8_t nWith2 = 0) override;
	bool canAnGangWithCard(uint8_t nCard)override;
	bool canBuGangWithCard(uint8_t nCard)override;
	void onVisitPlayerCardInfo(Json::Value& js, bool isSelf)override;
	virtual void onVisitPlayerCardBaseInfo(Json::Value& js);
	bool getHoldCardThatCanAnGang(VEC_CARD& vGangCards)override;
	bool getHoldCardThatCanBuGang(VEC_CARD& vGangCards)override;

	bool canHuWitCard( uint8_t nCard) override;
	bool isHoldCardCanHu( uint8_t& nJiang ) override;
	bool isTingPai() override;
	bool isTing() override { return false; }

	void onMoCard(uint8_t nMoCard) final;
	bool onPeng(uint8_t nCard, uint16_t nInvokerIdx ) final;
	bool onDirectGang(uint8_t nCard, uint8_t nGangGetCard, uint16_t nInvokerIdx ) override;
	bool onAnGang(uint8_t nCard, uint8_t nGangGetCard) override;
	bool onBuGang(uint8_t nCard, uint8_t nGangGetCard ) final;
	bool onEat(uint8_t nCard, uint8_t nWithA, uint8_t withB) final;
	bool onChuCard(uint8_t nChuCard)override;
	bool onBuHua(uint8_t nHuaCard, uint8_t nGetCard)override;

	bool getHoldCard(VEC_CARD& vHoldCard) final;
	bool getChuedCard(VEC_CARD& vChuedCard) final;
	bool getAnGangedCard(VEC_CARD& vAnGanged)final;
	bool getMingGangedCard(VEC_CARD& vGangCard) final;
	bool getDirectGangCard(VEC_CARD& vGangCard );
	bool getBuGangCard(VEC_CARD& vGangCard);
	bool getPengedCard(VEC_CARD& vPengedCard) final;
	bool getEatedCard(VEC_CARD& vEatedCard) final;
	bool getBuHuaCard(VEC_CARD& vHuaCard)final;
	uint8_t getHuaCard()override;

	uint32_t getNewestFetchedCard()final;
	void addLouPengedCard( uint8_t nLouPengedCard )final;
	uint8_t getJiang();

	bool CheckHoldCardAllShun(uint8_t nBaiDaCnt = 0);
protected:
	void addHoldCard( uint8_t nCard );
	void removeHoldCard(uint8_t nCard);
	virtual bool isCardTypeMustKeZi(uint8_t nCardType);
private:
	bool canHoldCard7PairHu(uint8_t& nJiang, uint8_t nBaiDaCnt);
	bool isHoldCardCanHuNormal(uint8_t& nJiang, uint8_t nBaiDaCnt);  // must not be override 
	uint8_t tryToFindMiniQueCnt(VEC_CARD vWaitCheck, bool isMustKeZi, VEC_CARD& vNotShun, uint8_t& nQueCnt, uint8_t nMaxQueCnt);
public:
	void debugCardInfo();
protected:
	uint16_t m_nDianPaoIdx;
	VEC_CARD m_vCards[eCT_Max];
	VEC_CARD m_vChuedCard;

	VEC_INVOKE_ACT_INFO m_vMingCardInfo;

	uint8_t m_nNesetFetchedCard;
	uint8_t m_nJIang;
	bool m_b7Pair = false;

	VEC_CARD m_vLouPenged;
};