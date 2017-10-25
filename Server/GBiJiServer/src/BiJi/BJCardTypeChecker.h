#pragma once
#include "BJDefine.h"
#include "BJPoker.h"
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
		std::sort(vecCards.begin(), vecCards.end());
		// make weight ;
		cardType = CardType_None;
		nWeight = (cardType << 16) | ((BJ_PARSE_VALUE(vecCards.back())) << 12) | (BJ_PARSE_TYPE(vecCards.back()));
		return true;
	}
};

class BJCardTypePair
	:public IBJCardType
{
public:
	bool isThisCardType( std::vector<uint8_t>& vecCards, uint32_t& nWeight, eBJCardType& cardType )override
	{
		std::sort(vecCards.begin(), vecCards.end());
		std::vector<uint8_t> vFaceValue;
		for (auto& ref : vecCards)
		{
			if ( BJ_PARSE_TYPE(ref) == ePoker_Joker ) // pick out joker
			{
				continue;
			}

			auto nV = BJ_PARSE_VALUE(ref);
			vFaceValue.push_back((nV == 1 ? 14 : nV));
		}

		bool isPair = false;
		uint8_t nPairValue = 0;
		uint8_t nSingleValue = 0;
		if (vFaceValue.size() < 3 )  // have joker 
		{
			isPair = true;
			if ( vFaceValue.size() == 1 ) // have two joker 
			{
				nPairValue = 14;   // joker represent A ;
			}
			else
			{
				nPairValue = vFaceValue.back();  // when have only one joker
			}
			nSingleValue = vFaceValue.front();
		}
		else
		{
			isPair = vFaceValue[0] == vFaceValue[1] || vFaceValue[1] == vFaceValue[2];
			nPairValue = vFaceValue[1];
			nSingleValue  = vFaceValue[0] == vFaceValue[1] ? vFaceValue[2] : vFaceValue[0];
		}
		 
		if ( !isPair )
		{
			return false;
		}

		// make weight ;
		cardType = CardType_Pair;
		uint8_t nSingleType = 0;
		for (auto& ref : vecCards)
		{
			if ( BJ_PARSE_VALUE(ref) == nSingleValue)
			{
				nSingleType = BJ_PARSE_TYPE(ref);
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
		std::sort(vecCards.begin(), vecCards.end());
		std::vector<uint8_t> vFaceValue;
		for (auto& ref : vecCards)
		{
			if (BJ_PARSE_TYPE(ref) == ePoker_Joker) // pick out joker
			{
				continue;
			}
			auto nV = BJ_PARSE_VALUE(ref);
			vFaceValue.push_back((nV == 1 ? 14 : nV ) );
		}

		// consider joker
		if ( vFaceValue.size() == 1 ) // have two joker
		{
			auto nV = vFaceValue.back();
			auto nV2 = nV + 1;
			auto nV3 = nV2 + 1;
			if ( nV3 > 14 )
			{
				nV = 12;
				nV2 = 13;
				nV3 = 14;
			}

			vFaceValue.clear();
			vFaceValue.push_back(nV);
			vFaceValue.push_back(nV2);
			vFaceValue.push_back(nV3);
		}
		else if ( vFaceValue.size() == 2 )  // have one joker 
		{
			auto nElaps = vFaceValue.back() - vFaceValue.front();
			uint8_t nJokeValue = 14; // default ;
			if ( nElaps == 0 )
			{
				nJokeValue = vFaceValue.back() + 1;
				if ( nJokeValue > 14)
				{
					nJokeValue = vFaceValue.front() - 1;
				}
			}
			else if (1 == nElaps)
			{
				nJokeValue = vFaceValue.front() + 1;
			}
			vFaceValue.push_back(nJokeValue);
			std::sort(vFaceValue.begin(),vFaceValue.end());
		}

		bool isShun = (vFaceValue[0] + 1) == vFaceValue[1] && (vFaceValue[1] + 1 == vFaceValue[2]);
		if (!isShun)
		{
			if (vFaceValue[0] == 2 && vFaceValue[1] == 3 && vFaceValue[2] == 14) // 123 shun 
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
		uint8_t nSingleType = ePoker_Sword;  // when can not find value , default must be joker , so joker should be biggest type ;
		for (auto& ref : vecCards)
		{
			if ( BJ_PARSE_VALUE(ref) == nSingleValue)
			{
				nSingleType = BJ_PARSE_TYPE(ref);
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
		std::sort(vecCards.begin(), vecCards.end());		
		uint8_t  nSinglCardType = BJ_PARSE_TYPE(vecCards.front()); // front can not be joker ;joker value is 15 , 16 ;
		// check is same color ignore joker 
		for (auto& ref : vecCards)
		{
			if ( BJ_PARSE_TYPE(ref) != ePoker_Joker && nSinglCardType != BJ_PARSE_TYPE(ref) ) // all card type be the same as front , or joker , joker can be any type ;
			{
				return false;
			}
		}

		std::vector<uint8_t> vFaceValue;
		for (auto& ref : vecCards)
		{
			if (BJ_PARSE_TYPE(ref) == ePoker_Joker) 
			{
				vFaceValue.push_back(14); // in same color ,joker face value must A ;
				continue;
			}
			auto nV = BJ_PARSE_VALUE(ref);
			vFaceValue.push_back((nV == 1 ? 14 : nV));
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
		std::sort(vecCards.begin(), vecCards.end());
		std::vector<uint8_t> vFaceValue;
		for (auto& ref : vecCards)
		{
			if (BJ_PARSE_TYPE(ref) == ePoker_Joker) // pick out joker
			{
				continue;
			}
			auto nV = BJ_PARSE_VALUE(ref);
			vFaceValue.push_back((nV == 1 ? 14 : nV));
		}

		if ( vFaceValue.front() != vFaceValue.back() )
		{
			return false;
		}

		// find max type 
		cardType = CardType_Bomb;
		nWeight = (cardType << 16) | (vFaceValue.front() << 12) | (vFaceValue.front() << 8) | (vFaceValue.front() << 4) | BJ_PARSE_TYPE(vecCards.back());
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