#pragma once
#include "CardPoker.h"
#include "DouDiZhuDefine.h"
#include <algorithm>
#include "Singleton.h"
class IDouDiZhuCardType
{
public:
	virtual bool isThisType( std::vector<uint8_t>& vCards , uint8_t& nWeight, DDZ_Type& eType ) = 0;
};

class IDouDiZhuRokect
	:public IDouDiZhuCardType
{
public:
	bool isThisType(std::vector<uint8_t>& vCards, uint8_t& nWeight, DDZ_Type& eType )override
	{
		if ( vCards.size() != 2 )
		{
			return false;
		}

		if ( POKER_PARSE_TYPE(vCards.front()) != ePoker_Joker || ePoker_Joker != POKER_PARSE_TYPE(vCards.back()) )
		{
			return false;
		}

		nWeight = 1;
		eType = DDZ_Rokect;
		return true;
	} 
};

class IDouDiZhuBomb
	:public IDouDiZhuCardType
{
public:
	bool isThisType(std::vector<uint8_t>& vCards, uint8_t& nWeight, DDZ_Type& eType)override
	{
		if (vCards.size() != 4)
		{
			return false;
		}
		
		std::sort(vCards.begin(),vCards.end());
		if (POKER_PARSE_VALUE(vCards.front()) != POKER_PARSE_VALUE(vCards.back()))
		{
			return false;
		}

		nWeight = POKER_PARSE_VALUE(vCards.front());
		eType = DDZ_Bomb;
		return true;
	}
};

class IDouDiZhuSingle
	:public IDouDiZhuCardType
{
public:
	bool isThisType(std::vector<uint8_t>& vCards, uint8_t& nWeight, DDZ_Type& eType)override
	{
		if (vCards.size() != 1)
		{
			return false;
		}
		eType = DDZ_Single;
		nWeight = POKER_PARSE_VALUE(vCards.front());
		if (POKER_PARSE_TYPE(vCards.front()) == ePoker_Joker)
		{
			nWeight = 100 + nWeight;  
		}
		else if (nWeight <= 2)   // A , 2 
		{
			nWeight += 80;
		}
		return true;
	}
};

class IDouDiZhuPair
	:public IDouDiZhuCardType
{
public:
	bool isThisType(std::vector<uint8_t>& vCards, uint8_t& nWeight, DDZ_Type& eType)override
	{
		if (vCards.size() != 2)
		{
			return false;
		}

		if (POKER_PARSE_VALUE(vCards.front()) != POKER_PARSE_VALUE(vCards.back()))
		{
			return false;
		}

		eType = DDZ_Pair;
		nWeight = POKER_PARSE_VALUE(vCards.front());
		return true;
	}
};

class IDouDiZhu3Pices
	:public IDouDiZhuCardType
{
public:
	bool isThisType(std::vector<uint8_t>& vCards, uint8_t& nWeight, DDZ_Type& eType)override
	{
		if (vCards.size() != 3)
		{
			return false;
		}

		std::sort(vCards.begin(),vCards.end());
		if (POKER_PARSE_VALUE(vCards.front()) != POKER_PARSE_VALUE(vCards.back()))
		{
			return false;
		}

		eType = DDZ_3Pices;
		nWeight = POKER_PARSE_VALUE(vCards.front());
		return true;
	}
};

class IDouDiZhu3Follow1
	:public IDouDiZhuCardType
{
public:
	bool isThisType(std::vector<uint8_t>& vCards, uint8_t& nWeight, DDZ_Type& eType)override
	{
		if ( vCards.size() != 4 && vCards.size() != 5 )
		{
			return false;
		}

		std::sort(vCards.begin(),vCards.end());
		if ( POKER_PARSE_VALUE(vCards[0u]) == POKER_PARSE_VALUE(vCards[2u]) )
		{
			if ( vCards.size() == 5 && POKER_PARSE_VALUE(vCards[3u]) != POKER_PARSE_VALUE(vCards[4u]) )
			{
				return false;
			}
			nWeight = POKER_PARSE_VALUE(vCards[0u]);
		}
		else if (POKER_PARSE_VALUE(vCards[vCards.size() - 1 ]) == POKER_PARSE_VALUE(vCards[vCards.size() - 3u]) )
		{
			if (vCards.size() == 5 && POKER_PARSE_VALUE(vCards[0]) != POKER_PARSE_VALUE(vCards[1]))
			{
				return false;
			}

			nWeight = POKER_PARSE_VALUE(vCards[vCards.size() - 1]);
		}
		else
		{
			return false;
		}
		eType = DDZ_3Follow1;
		return true;
	}
};

class IDouDiZhuSingleSequence
	:public IDouDiZhuCardType
{
public:
	bool isThisType(std::vector<uint8_t>& vCards, uint8_t& nWeight, DDZ_Type& eType)override
	{
		if (vCards.size() < 5)
		{
			return false;
		}

		std::sort(vCards.begin(), vCards.end());
		bool isFirstA = POKER_PARSE_VALUE(vCards[0]) == 1 ;
		for ( uint8_t nIdx = isFirstA ? 1 : 0 ; (nIdx + 1u) < vCards.size(); ++nIdx )
		{
			if ( 2 == POKER_PARSE_VALUE(vCards[nIdx]) || ePoker_Joker == POKER_PARSE_TYPE(vCards[nIdx]) || (POKER_PARSE_VALUE(vCards[nIdx]) + 1) != POKER_PARSE_VALUE(vCards[nIdx + 1]))
			{
				return false;
			}
		}

		if (isFirstA && POKER_PARSE_VALUE(vCards.back()) != 13)
		{
			return false;
		}

		nWeight = isFirstA ? POKER_PARSE_VALUE(vCards[1u]) : POKER_PARSE_VALUE(vCards[0u]);
		eType = DDZ_SingleSequence;
		return true;
	}
};

class IDouDiZhuPairSequence
	:public IDouDiZhuCardType
{
public:
	bool isThisType( std::vector<uint8_t>& vCards, uint8_t& nWeight, DDZ_Type& eType)override
	{
		if (vCards.size() < 6 || vCards.size() % 2 != 0)
		{
			return true;
		}

		std::vector<uint8_t> vTemp;
		for (auto& ref : vCards)
		{
			if (POKER_PARSE_TYPE(ref) == ePoker_Joker)
			{
				return false;
			}

			auto tValue = POKER_PARSE_VALUE(ref);
			if (2 == tValue)
			{
				return false;
			}

			if ( 1 == tValue)
			{
				tValue = 14;
			}

			vTemp.push_back(tValue);
		}

		std::sort(vTemp.begin(),vTemp.end() );
		for ( uint8_t nIdx = 0; (nIdx + 2) < vTemp.size(); nIdx += 2)
		{
			if (vTemp[nIdx] + 1 != vTemp[nIdx + 2])
			{
				return false;
			}
		}
		nWeight = vTemp[0];
		eType = DDZ_PairSequence;
		return true;
	}
};

class IDouDiZhu3PicesSeqence
	:public IDouDiZhuCardType
{
public:
	bool isThisType(std::vector<uint8_t>& vCards, uint8_t& nWeight, DDZ_Type& eType)override
	{
		if (vCards.size() < 6 || vCards.size() % 3 != 0)
		{
			return true;
		}

		std::vector<uint8_t> vTemp;
		for (auto& ref : vCards)
		{
			if (POKER_PARSE_TYPE(ref) == ePoker_Joker)
			{
				return false;
			}

			auto tValue = POKER_PARSE_VALUE(ref);
			if (2 == tValue)
			{
				return false;
			}

			if (1 == tValue)
			{
				tValue = 14;
			}

			vTemp.push_back(tValue);
		}

		std::sort(vTemp.begin(), vTemp.end());
		for (uint8_t nIdx = 0; (nIdx + 3) < vTemp.size(); nIdx += 3 )
		{
			if (vTemp[nIdx] + 1 != vTemp[nIdx + 3])
			{
				return false;
			}
		}
		nWeight = vTemp[0];
		eType = DDZ_3PicesSeqence;
		return true;
	}
};

class IDouDiZhuAircraftWithWings
	:public IDouDiZhuCardType
{
public:
	bool isThisType(std::vector<uint8_t>& vCards, uint8_t& nWeight, DDZ_Type& eType)override
	{
		std::sort(vCards.begin(), vCards.end());
		std::vector<uint8_t> v3,vOther;
		for ( uint8_t nIdx = 0; nIdx < vCards.size(); )
		{
			if ( (nIdx + 2) < vCards.size() && POKER_PARSE_VALUE(vCards[nIdx]) == POKER_PARSE_VALUE(vCards[nIdx + 2]))
			{
				v3.push_back(vCards[nIdx]);
				v3.push_back(vCards[nIdx]);
				v3.push_back(vCards[nIdx]);
				nIdx += 3;
			}
			else
			{
				vOther.push_back(POKER_PARSE_VALUE(vCards[nIdx]));
				++nIdx;
			}
		}

		IDouDiZhu3PicesSeqence t3Shun;
		uint8_t nTempWeight;
		DDZ_Type eT;
		if (t3Shun.isThisType(v3, nTempWeight, eT) == false)
		{
			return false;
		}

		auto nShunCnt = v3.size() / 3;
		if ( nShunCnt != vOther.size() && nShunCnt * 2 != vOther.size() )
		{
			return false;
		}

		if (nShunCnt * 2 == vOther.size()) // other must pair 
		{
			for (uint8_t nIdx = 0; (nIdx + 1) < vOther.size(); nIdx += 2)
			{
				if (vOther[nIdx] != vOther[nIdx + 1])
				{
					return false;
				}
			}
		}

		nWeight = nTempWeight;
		eType = DDZ_AircraftWithWings;
		return true;
	}
};

class IDouDiZhu4Follow2
	:public IDouDiZhuCardType
{
public:
	bool isThisType(std::vector<uint8_t>& vCards, uint8_t& nWeight, DDZ_Type& eType)override
	{
		if (vCards.size() != 6 && vCards.size() != 8)
		{
			return false;
		}

		std::sort(vCards.begin(), vCards.end());
		// not finish 
		uint8_t n4CardValue = 0;
		std::vector<uint8_t> vOther;
		for (uint8_t nIdx = 0; nIdx < vCards.size(); )
		{
			if ((nIdx + 3) < vCards.size() && POKER_PARSE_VALUE(vCards[nIdx]) == POKER_PARSE_VALUE(vCards[nIdx + 3]))
			{
				n4CardValue = POKER_PARSE_VALUE(vCards[nIdx]);
				nIdx += 4;
			}
			else
			{
				vOther.push_back(POKER_PARSE_VALUE(vCards[nIdx]));
				++nIdx;
			}
		}

		if (0 == n4CardValue)
		{
			return false;
		}

		if ( vOther.size() == 4 && vOther.size() == 4 && (vOther[0] != vOther[1] || vOther[2] != vOther[3]) )
		{
			return false;
		}

		nWeight = n4CardValue;
		eType = DDZ_4Follow2;
		return true;
	}
};

// dou di zhu checker 
class DDZCardTypeChecker
	:public CSingleton<DDZCardTypeChecker>
{
public:
	DDZCardTypeChecker()
	{
		IDouDiZhuCardType* p = new IDouDiZhuRokect();
		m_vCardTypes.push_back(p);

		p = new IDouDiZhuBomb();
		m_vCardTypes.push_back(p);

		p = new IDouDiZhuSingle();
		m_vCardTypes.push_back(p);

		p = new IDouDiZhuPair();
		m_vCardTypes.push_back(p);

		p = new IDouDiZhu3Pices();
		m_vCardTypes.push_back(p);

		p = new IDouDiZhu3Follow1();
		m_vCardTypes.push_back(p);

		p = new IDouDiZhuSingleSequence();
		m_vCardTypes.push_back(p);

		p = new IDouDiZhuPairSequence();
		m_vCardTypes.push_back(p);

		p = new IDouDiZhu3PicesSeqence();
		m_vCardTypes.push_back(p);

		p = new IDouDiZhuAircraftWithWings();
		m_vCardTypes.push_back(p);

		p = new IDouDiZhu4Follow2();
		m_vCardTypes.push_back(p);
	}

public:
	~DDZCardTypeChecker()
	{
		for (auto& ref : m_vCardTypes)
		{
			delete ref;
			ref = nullptr;
		}
		m_vCardTypes.clear();
	}
public:
	bool checkCardType(std::vector<uint8_t>& vecCards, uint8_t& nWeight, DDZ_Type & cardType)
	{
		for (auto& ref : m_vCardTypes)
		{
			if (ref->isThisType(vecCards, nWeight, cardType))
			{
				return true;
			}
		}
		return false;
	}
	bool isCardTypeWeightValid(std::vector<uint8_t>& vecCards, uint8_t nWeight, DDZ_Type cardType )
	{
		uint8_t nWeightTemp = 0; DDZ_Type cardTypeTemp = DDZ_Max;
		if (false == checkCardType(vecCards, nWeightTemp, cardTypeTemp))
		{
			return false;
		}
		return nWeightTemp == nWeight && cardType == cardTypeTemp;
	}
protected:
	std::vector<IDouDiZhuCardType*> m_vCardTypes;
};