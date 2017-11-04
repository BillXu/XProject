#pragma once
#include "CardPoker.h"
#include "DouDiZhuDefine.h"
#include <algorithm>
#include "Singleton.h"
class IDouDiZhuCardType
{
public:
	virtual bool isThisType( std::vector<uint8_t>& vCards , uint8_t& nWeight, DDZ_Type& eType ) = 0;
	static bool pickOutCardGroupsAndErase( std::vector<uint8_t>& vCards, uint8_t nCntPerGroup, std::vector<uint8_t>& vOut)
	{
		std::sort(vCards.begin(),vCards.end());
		for ( uint8_t nIdx = 0; (nIdx + nCntPerGroup - 1) < vCards.size() && (vCards.size() - nIdx) >= nCntPerGroup; )
		{
			if ( DDZ_PARSE_VALUE(vCards[nIdx]) == DDZ_PARSE_VALUE(vCards[nIdx + nCntPerGroup - 1]))
			{
				auto nCnt = nCntPerGroup;
				while (nCnt--)
				{
					vOut.push_back(vCards[nIdx++]);
				}
				continue;
			}
			++nIdx;
		}
		vCards.erase(vOut.begin(),vOut.end());
		return vOut.empty() == false;
	}

	static bool pickOutShunGroups( std::vector<uint8_t> vCards, uint8_t nCntPerGroup,uint8_t nShunSize, std::vector<uint8_t>& vOut )
	{
		std::vector<uint8_t> vGrouped;
		if ( pickOutCardGroupsAndErase(vCards, nCntPerGroup, vGrouped) == false )
		{
			return false;
		}

		// decrase order ;
		std::sort(vGrouped.begin(), vGrouped.end(), [](uint8_t& left, uint8_t& right) { return left > right; });
		// sort groups 
		uint8_t nLastShunValue = 0;
		for ( uint8_t nIdx = 0; (nIdx + nCntPerGroup - 1) < vGrouped.size(); nIdx += nCntPerGroup )
		{
			auto nValue = DDZ_PARSE_VALUE(vGrouped[nIdx]);

			if ( nLastShunValue == nValue )
			{
				continue;
			}

			if ( nLastShunValue != 0 && ( nLastShunValue - 1 != nValue ) )
			{
				vOut.clear();
			}
			nLastShunValue = nValue;

			for (uint8_t nAddIdx = 0; nAddIdx < nCntPerGroup; ++nAddIdx)
			{
				vOut.push_back(vGrouped[nIdx + nAddIdx]);
			}

			if ( vOut.size() == nShunSize * nCntPerGroup)
			{
				std::sort(vOut.begin(),vOut.end()); 
				return true;
			}
		}
		vOut.clear();
		return false;
	}
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

		if ( DDZ_PARSE_TYPE(vCards.front()) != ePoker_Joker || ePoker_Joker != DDZ_PARSE_TYPE(vCards.back()) )
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
		if (DDZ_PARSE_VALUE(vCards.front()) != DDZ_PARSE_VALUE(vCards.back()))
		{
			return false;
		}

		nWeight = DDZ_PARSE_VALUE(vCards.front());
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
		nWeight = DDZ_PARSE_VALUE(vCards.front());
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

		if (DDZ_PARSE_VALUE(vCards.front()) != DDZ_PARSE_VALUE(vCards.back()))
		{
			return false;
		}

		eType = DDZ_Pair;
		nWeight = DDZ_PARSE_VALUE(vCards.front());
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
		if (DDZ_PARSE_VALUE(vCards.front()) != DDZ_PARSE_VALUE(vCards.back()))
		{
			return false;
		}

		eType = DDZ_3Pices;
		nWeight = DDZ_PARSE_VALUE(vCards.front());
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

		auto tmp = vCards;
		decltype(tmp) vOut;
		if (pickOutCardGroupsAndErase(tmp, 3, vOut) == false)
		{
			return false;
		}

		if (tmp.size() == 2) // must pair 
		{
			if (DDZ_PARSE_VALUE(tmp[0]) != DDZ_PARSE_VALUE(tmp[1]))
			{
				return false;
			}
		}
		
		nWeight = vOut.front();
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

		std::vector<uint8_t> vOutShun;
		if (pickOutShunGroups(vCards, 1, vCards.size() , vOutShun) == false)
		{
			return false;
		}

		if (vOutShun.size() != vCards.size())
		{
			return false;
		}
		nWeight = vOutShun.front();
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
			return false;
		}

		std::vector<uint8_t> vOutShun;
		if (pickOutShunGroups(vCards, 2, vCards.size() / 2, vOutShun) == false)
		{
			return false;
		}

		if (vOutShun.size() != vCards.size())
		{
			return false;
		}

		nWeight = vOutShun.front();
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
			return false;
		}

		std::vector<uint8_t> vOutShun;
		if (pickOutShunGroups(vCards, 3, vCards.size() / 3 , vOutShun) == false)
		{
			return false;
		}
		
		if ( vOutShun.size() != vCards.size() )
		{
			return false;
		}

		nWeight = vOutShun.front();
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
		std::vector<uint8_t> vGroups;
		auto vTmp = vCards;
		if ( pickOutCardGroupsAndErase(vTmp, 3, vGroups) == false )
		{
			return false;
		}

		if ( vGroups.size() < 2 )
		{
			return false;
		}

		uint8_t nMaxGroupCnt = vGroups.size();
		while ( nMaxGroupCnt >= 2 )
		{
			// check card count order 
			if ((nMaxGroupCnt * 3 + nMaxGroupCnt) != vCards.size() && vCards.size() != (nMaxGroupCnt * 3 + nMaxGroupCnt * 2))
			{
				--nMaxGroupCnt;
				continue;
			}

			std::vector<uint8_t> vOutShun;
			if (pickOutShunGroups(vGroups, 3, nMaxGroupCnt, vOutShun) == false)
			{
				--nMaxGroupCnt;
				continue;
			}

			// do find lian shun , erase hold ;
			auto vFollow = vCards;
			vFollow.erase(vOutShun.begin(),vOutShun.end());
			bool isFollowPair = vCards.size() == (nMaxGroupCnt * 3 + nMaxGroupCnt * 2);
			if ( isFollowPair )
			{
				std::vector<uint8_t> vPairs;
				if (pickOutCardGroupsAndErase(vFollow, 2, vPairs) == false )
				{
					--nMaxGroupCnt;
					continue;
				}

				if (vPairs.size() != 2 * nMaxGroupCnt)
				{
					--nMaxGroupCnt;
					continue;
				}
			}
			
			// do 
			nWeight = vOutShun.front();
			eType = DDZ_AircraftWithWings;
			return true;
		}
		return false;
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

		std::vector<uint8_t> vGroups;
		auto vTmp = vCards;
		if (pickOutCardGroupsAndErase(vTmp, 4, vGroups) == false)
		{
			return false;
		}

		if ( vTmp.size() == 4 ) // must tow pair ;
		{
			decltype(vGroups) vPairs;
			if (pickOutCardGroupsAndErase(vTmp, 2, vPairs ) == false)
			{
				return false;
			}

			if (vPairs.size() != 4)
			{
				return false;
			}
		}
		nWeight = vGroups.front();
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
		m_vCardTypes[DDZ_Rokect] = p;

		p = new IDouDiZhuBomb();
		m_vCardTypes[DDZ_Bomb] = p;

		p = new IDouDiZhuSingle();
		m_vCardTypes[DDZ_Single] = p;

		p = new IDouDiZhuPair();
		m_vCardTypes[DDZ_Pair] = p;

		p = new IDouDiZhu3Pices();
		m_vCardTypes[DDZ_3Pices] = p;

		p = new IDouDiZhu3Follow1();
		m_vCardTypes[DDZ_3Follow1] = p;

		p = new IDouDiZhuSingleSequence();
		m_vCardTypes[DDZ_SingleSequence] = p;

		p = new IDouDiZhuPairSequence();
		m_vCardTypes[DDZ_PairSequence] = p;

		p = new IDouDiZhu3PicesSeqence();
		m_vCardTypes[DDZ_3PicesSeqence] = p;

		p = new IDouDiZhuAircraftWithWings();
		m_vCardTypes[DDZ_AircraftWithWings] = p;

		p = new IDouDiZhu4Follow2();
		m_vCardTypes[DDZ_4Follow2] = p;
	}

public:
	~DDZCardTypeChecker()
	{
		for (auto& ref : m_vCardTypes)
		{
			delete ref.second;
			ref.second = nullptr;
		}
		m_vCardTypes.clear();
	}
public:
	bool isCardTypeValid(std::vector<uint8_t>& vecCards, DDZ_Type cardType, uint8_t& nWeight )
	{
		auto iter = m_vCardTypes.find(cardType);
		if (iter == m_vCardTypes.end())
		{
			return false;
		}

		DDZ_Type cardTypeTemp = DDZ_Max;
		if ( false == iter->second->isThisType(vecCards, nWeight, cardTypeTemp))
		{
			return false;
		}
		return true;
	}
protected:
	std::map<DDZ_Type,IDouDiZhuCardType*> m_vCardTypes;
};