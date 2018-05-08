#pragma once
#include "IPeerCard.h"
#include "CardPoker.h"
#include "CommonDefine.h"
#include "json\json.h"
class CGoldenPeerCard
	:public IPeerCard
{
public:
	enum  GoldenType
	{
		Golden_None,
		Golden_Single,//单张
		Golden_Double,//对子
		Golden_Straight,//顺子
		Golden_Flush,//同花
		Golden_StraightFlush,//同花顺
		Golden_ThreeCards,//豹子
		Golden_Max,
	};
public:
    CGoldenPeerCard();
	void setOpts( bool isEnable235, bool isEnableStraightWin );
	void addCompositCardNum( uint8_t nCardCompositNum ) override ;
	const char*  getNameString() override;
	uint32_t getWeight() override;
	uint8_t getType();
	void reset() override ;
	PK_RESULT pk(IPeerCard* pTarget) override;
    uint8_t getAddIdx(){return m_nAddIdx;}
	uint8_t getHoldCardCnt() { return m_nAddIdx; }
	void toJson( Json::Value& js );
	bool check235();
	CGoldenPeerCard& operator = (CGoldenPeerCard& pRight )
	{
		for( uint8_t nIdx = 0 ; nIdx < GOLDEN_HOLD_CARD_COUNT; ++nIdx )
		{
			uint8_t nRight = pRight.m_vHoldCards[nIdx].GetCardCompositeNum() ;
			m_vHoldCards[nIdx].RsetCardByCompositeNum(nRight);
		}

		m_nAddIdx = pRight.m_nAddIdx ;
		m_eType = pRight.m_eType ;
		m_bEnable235 = pRight.m_bEnable235;
		m_bEnableStraightWin = pRight.m_bEnableStraightWin;
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
		return getHoldCardCnt();
	}

	IPeerCard* swap(IPeerCard* pTarget)override
	{
		auto pT = dynamic_cast<CGoldenPeerCard*>(pTarget);
		assert(pT && "why target golden card is null?");
		CGoldenPeerCard tTemp ;
		tTemp = *this ;
		*this = *pT ;
		*pT = tTemp ;
		return this ;
	}

	void addCardToVecAsc(std::vector<CCard>& vec, CCard cCard, bool specialA = true)
	{
		auto iter = vec.begin();
		for (; iter < vec.end(); ++iter)
		{
			if ((*iter).GetCardFaceNum(specialA) < cCard.GetCardFaceNum(specialA))
			{
				vec.insert(iter, cCard);
				return;
			}
			else if ((*iter).GetCardFaceNum(specialA) == cCard.GetCardFaceNum(specialA))
			{
				if ((*iter).GetType() < cCard.GetType()) {
					vec.insert(iter, cCard);
					return;
				}
			}
		}
		vec.push_back(cCard);
	}
	
protected:
	bool isCaculated();
	void caculateCards();
	void caculateCards(GoldenType& type, std::vector<CCard> vHoldCards);
	bool checkThreeCards(std::vector<CCard> vHoldCards);
	bool checkStraightFlush(std::vector<CCard> vHoldCards);
	bool checkFlush(std::vector<CCard> vHoldCards);
	bool checkStraight(std::vector<CCard> vHoldCards);
	bool checkDouble(std::vector<CCard> vHoldCards);
	bool isHaveJoker();
	uint8_t getCaculatedCards(std::vector<CCard>& vHoldCards);
protected:
	std::vector<CCard> m_vHoldCards ;
	uint8_t m_nAddIdx;

	GoldenType m_eType ;

	bool m_bEnable235;
	bool m_bEnableStraightWin;
};