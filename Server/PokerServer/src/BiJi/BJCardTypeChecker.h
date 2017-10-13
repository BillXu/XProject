#pragma once
#include "BJDefine.h"
#include "CardPoker.h"
#include <vector>
#include <algorithm>
#include "Singleton.h"
class IBJCardType
{
public:
	virtual bool isThisCardType( std::vector<uint8_t>& vecCards , uint32_t& nWeight,eBJCardType& cardType ) = 0 ;
};

// 权重定义：牌型值（3） | 重要面值1（4） | 重要面值2（4）| 重要面值3（4）| 重要牌花色类型(4)， 总共32位
// 1,2 ,3 是最小的顺子
class BJCardTypeNone
	:public IBJCardType
{
public:
	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, eBJCardType& cardType)override
	{
		CCard tCard;
		std::vector<uint8_t> vFaceValue;
		for (auto& ref : vecCards)
		{
			tCard.RsetCardByCompositeNum(ref);
			vFaceValue.push_back(tCard.GetCardFaceNum(true));
		}
		std::sort(vFaceValue.begin(), vFaceValue.end());

		// make weight ;
		cardType = CardType_None;
		uint8_t nSingleValue = vFaceValue[2];
		uint8_t nSingleType = 0;
		for (auto& ref : vecCards)
		{
			tCard.RsetCardByCompositeNum(ref);
			if (tCard.GetCardFaceNum() == nSingleValue)
			{
				nSingleType = tCard.GetType();
				break;
			}
		}

		nWeight = (cardType << 16) | (vFaceValue[2] << 12) | (vFaceValue[1] << 8) | (vFaceValue[0] << 4) | nSingleType;
		return true;
	}
};

class BJCardTypePair
	:public IBJCardType
{
public:
	bool isThisCardType( std::vector<uint8_t>& vecCards, uint32_t& nWeight, eBJCardType& cardType )override
	{
		CCard tCard;
		std::vector<uint8_t> vFaceValue;
		for ( auto& ref : vecCards )
		{
			tCard.RsetCardByCompositeNum(ref);
			vFaceValue.push_back(tCard.GetCardFaceNum(true));
		}
		std::sort(vFaceValue.begin(),vFaceValue.end());

		bool isPair = vFaceValue[0] == vFaceValue[1] || vFaceValue[1] == vFaceValue[2];
		if ( !isPair )
		{
			return false;
		}

		// make weight ;
		uint8_t nPairValue = vFaceValue[1];
		cardType = CardType_Pair;
		uint8_t nSingleValue = vFaceValue[0] == vFaceValue[1] ? vFaceValue[2] : vFaceValue[0];
		uint8_t nSingleType = 0;
		for (auto& ref : vecCards)
		{
			tCard.RsetCardByCompositeNum(ref);
			if (tCard.GetCardFaceNum() == nSingleValue)
			{
				nSingleType = tCard.GetType();
				break;
			}
		}

		nWeight = (cardType << 16) | (nPairValue << 12) | (nPairValue << 8) | (nSingleValue << 4) | nSingleType;
		return true;
	}
};

class BJCardTypeSequence
	:public IBJCardType
{
public:
	bool isThisCardType( std::vector<uint8_t>& vecCards, uint32_t& nWeight, eBJCardType& cardType )override
	{
		CCard tCard;
		std::vector<uint8_t> vFaceValue;
		for (auto& ref : vecCards)
		{
			tCard.RsetCardByCompositeNum(ref);
			vFaceValue.push_back(tCard.GetCardFaceNum(true));
		}
		std::sort(vFaceValue.begin(), vFaceValue.end());

		bool isShun = ( vFaceValue[0] + 1 ) == vFaceValue[1] && ( vFaceValue[1] + 1 == vFaceValue[2] );
		if ( !isShun )
		{
			if ( vFaceValue[0] == 2 && vFaceValue[1] == 3 && vFaceValue[2] == 14) // 123 shun 
			{
				isShun = true;
				vFaceValue[2] = 1;
				std::sort(vFaceValue.begin(), vFaceValue.end());
			}
		}
 
		if (!isShun)
		{
			return true;
		}

		// make weight ;
		cardType = CardType_Sequence;
		uint8_t nSingleValue = vFaceValue[2];
		uint8_t nSingleType = 0;
		for (auto& ref : vecCards)
		{
			tCard.RsetCardByCompositeNum(ref);
			if (tCard.GetCardFaceNum() == nSingleValue)
			{
				nSingleType = tCard.GetType();
				break;
			}
		}

		nWeight = (cardType << 16) | (vFaceValue[2] << 12) | (vFaceValue[1] << 8) | (vFaceValue[0] << 4) | nSingleType;
		return true;
	}
};

class BJCardTypeSameColor
	:public IBJCardType
{
public:
	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, eBJCardType& cardType )override
	{
		CCard tCard;
		uint8_t nSinglCardType = CCard::eCard_Max;
		for (auto& ref : vecCards)
		{
			tCard.RsetCardByCompositeNum(ref);
			if ( nSinglCardType == CCard::eCard_Max )
			{
				nSinglCardType = tCard.GetType();
				continue;
			}

			if (nSinglCardType != tCard.GetType())
			{
				return false;
			}
		}

		// sort face value ;
		std::vector<uint8_t> vFaceValue;
		for (auto& ref : vecCards)
		{
			tCard.RsetCardByCompositeNum(ref);
			vFaceValue.push_back(tCard.GetCardFaceNum(true));
		}
		std::sort(vFaceValue.begin(), vFaceValue.end());
		
		// make weight ;
		cardType = CardType_SameColor;
		nWeight = ( cardType << 16 ) | (vFaceValue[2] << 12 ) | (vFaceValue[1] << 8 ) | ( vFaceValue[0] << 4 ) | nSinglCardType ;
		return true;
	}
};

class BJCardTypeSameColorSequence
	:public IBJCardType
{
public:
	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, eBJCardType& cardType )override
	{
		BJCardTypeSameColor tSameColor;
		if (tSameColor.isThisCardType(vecCards, nWeight, cardType) == false)
		{
			return false;
		}

		BJCardTypeSequence tSeq;
		nWeight = 0;
		if (tSeq.isThisCardType(vecCards, nWeight, cardType))
		{
			cardType = CardType_SameColorSequence;
			nWeight &= 0xffff;
			nWeight |= ( cardType << 16 );
			return true;
		}
		return false;
	}
};

class BJCardTypeBomb
	:public IBJCardType
{
public:
	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, eBJCardType& cardType )override
	{
		CCard tCard;
		std::vector<uint8_t> vFaceValue;
		for (auto& ref : vecCards)
		{
			tCard.RsetCardByCompositeNum(ref);
			vFaceValue.push_back(tCard.GetCardFaceNum(true));
		}
		std::sort(vFaceValue.begin(), vFaceValue.end());
		if (vFaceValue[0] != vFaceValue[2])
		{
			return false;
		}

		// find max type 
		std::vector<uint8_t> vTypeValue;
		for (auto& ref : vecCards)
		{
			tCard.RsetCardByCompositeNum(ref);
			vTypeValue.push_back(tCard.GetType());
		}
		std::sort(vTypeValue.begin(), vTypeValue.end());

		cardType = CardType_Bomb;
		nWeight = (cardType << 16) | (vFaceValue[2] << 12) | (vFaceValue[1] << 8) | (vFaceValue[0] << 4) | vTypeValue[2];
		return true;
	}
};

class BJCardTypeChecker
	:public CSingleton<BJCardTypeChecker>
{
public:
	BJCardTypeChecker()
	{
		IBJCardType* p = new BJCardTypeBomb();
		m_vCardTypes.push_back(p);

		p = new BJCardTypeSameColorSequence();
		m_vCardTypes.push_back(p);

		p = new BJCardTypeSameColor();
		m_vCardTypes.push_back(p);

		p = new BJCardTypeSequence();
		m_vCardTypes.push_back(p);

		p = new BJCardTypePair();
		m_vCardTypes.push_back(p);

		p = new BJCardTypeNone();
		m_vCardTypes.push_back(p);
	}

public:
	~BJCardTypeChecker()
	{
		for (auto& ref : m_vCardTypes)
		{
			delete ref;
			ref = nullptr;
		}
		m_vCardTypes.clear();
	}
public:
	bool checkCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, eBJCardType& cardType)
	{
		if (vecCards.size() != 3)
		{
			return false;
		}

		for (auto& ref : m_vCardTypes)
		{
			if ( ref->isThisCardType( vecCards,nWeight,cardType) )
			{
				return true;
			}
		}
		return false;
	}
protected:
	std::vector<IBJCardType*> m_vCardTypes;
};