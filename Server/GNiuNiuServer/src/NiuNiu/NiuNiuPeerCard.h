#pragma once
#include "IPeerCard.h"
#include "CardPoker.h"
#include "CommonDefine.h"
#include "json\json.h"
class CNiuNiuPeerCard
	:public IPeerCard
{
public:
	struct CardGroup   
	{
		uint8_t nThreeIdx[3] ;
		uint8_t nTwoIdx[2];
		CardGroup(uint8_t tw0,uint8_t tw1, uint8_t th0 , uint8_t th1 , uint8_t th2 )
		{
			nTwoIdx[0] = tw0 ;
			nTwoIdx[1] = tw1 ;

			nThreeIdx[0] = th0 ;
			nThreeIdx[1] = th1 ;
			nThreeIdx[2] = th2 ;
		}
	};

	enum  NiuNiuType
	{
		Niu_None,
		Niu_Single,
		Niu_Niu,
		Niu_ShunZiNiu , ///
		Niu_TongHuaNiu,///
		Niu_Hulu,///
        Niu_FiveFlower,
		Niu_Boom,
		Niu_FiveSmall,
		Niu_TongHuaShun, ///
		Niu_Max,
	};
public:
    CNiuNiuPeerCard();
	void setOpts( bool isEnableFiveFlower, bool isEnableBoom, bool isEnableFiveSmall, bool isEnableShunKan, bool isEnableCrazy, bool isEnableHulu );
	void addCompositCardNum( uint8_t nCardCompositNum ) override ;
	const char*  getNameString() override;
	uint32_t getWeight() override;
	void reset() override ;
	NiuNiuType getType();
	uint8_t getPoint();
#ifndef SERVER
    CNiuNiuPeerCard::CardGroup getCardGroup();
#endif
    uint8_t getAddIdx(){return m_nAddIdx;}
	uint8_t getHoldCardCnt() { return m_nAddIdx; }
	void toJson( Json::Value& js );
	CNiuNiuPeerCard& operator = (CNiuNiuPeerCard& pRight )
	{
		for( uint8_t nIdx = 0 ; nIdx < NIUNIU_HOLD_CARD_COUNT; ++nIdx )
		{
			uint8_t nRight = pRight.m_vHoldCards[nIdx].GetCardCompositeNum() ;
			m_vHoldCards[nIdx].RsetCardByCompositeNum(nRight);
		}

		m_nAddIdx = pRight.m_nAddIdx ;
		m_nBiggestCardIdx = pRight.m_nBiggestCardIdx ;
		m_eType = pRight.m_eType ;
		m_nPoint = pRight.m_nPoint ;
		m_nGroupIdx = pRight.m_nGroupIdx ;
		m_nWeight = pRight.m_nWeight ;
		return *this ;
	}

	uint8_t getHoldCards( std::vector<uint8_t>& vHoldCards )
	{
		vHoldCards.clear();
		if ( 0 == m_nAddIdx )
		{
			return 0;
		}

		for (auto nIdx = 0; nIdx < getHoldCardCnt(); ++nIdx)
		{
			vHoldCards.push_back(m_vHoldCards[nIdx].GetCardCompositeNum());
		}
		return (uint8_t)vHoldCards.size();
	}

	IPeerCard* swap(IPeerCard* pTarget)override
	{
		auto pT = dynamic_cast<CNiuNiuPeerCard*>(pTarget);
		assert(pT && "why target niuniu card is null?");
		CNiuNiuPeerCard tTemp ;
		tTemp = *this ;
		*this = *pT ;
		*pT = tTemp ;
		return this ;
	}
	
protected:
	bool isCaculated();
	void caculateCards();
	void caculateCards( NiuNiuType& type , uint8_t& nPoint , uint32_t& nWeight, uint8_t& nGroupIdx, std::vector<CCard>& vHoldCards );
	bool checkNiu(std::vector<CCard>& vHoldCards,CardGroup& ref );
	bool checkFiveSmall(std::vector<CCard>& vHoldCards);
	bool checkFiveFlower(std::vector<CCard>& vHoldCards);
	bool checkBoom(std::vector<CCard>& vHoldCards);
	bool checkTongHuaShun(std::vector<CCard>& vHoldCards);
	bool checkHuLu(std::vector<CCard>& vHoldCards);
	bool checkTongHuaNiu(std::vector<CCard>& vHoldCards);
	bool checkShunZiNiu(std::vector<CCard>& vHoldCards);
	bool isHaveJoker();
	void caculateNiuNiuCards( NiuNiuType& type, uint8_t& nPoint, uint32_t& nWeight, uint8_t& nGroupIdx, std::vector<CCard> vHoldCards );
protected:
	static CardGroup s_CardGroup[10] ;
protected:
	std::vector<CCard> m_vHoldCards ;
	uint8_t m_nAddIdx ;
	uint8_t m_nBiggestCardIdx ;

	NiuNiuType m_eType ;
	uint8_t m_nPoint ;
	uint8_t m_nGroupIdx ;
	uint32_t m_nWeight ;

	bool m_isEnableFiveFlower;
	bool m_isEnableBoom;
	bool m_isEnableFiveSmall;
	bool m_isEnableShunKan;
	bool m_isEnableCrazy;
	bool m_isEnableHulu;
};