#pragma once
#include "CardPoker.h"
#include "DouDiZhuDefine.h"
#include <algorithm>
#include "Singleton.h"
class IDDZFindCardType
{
public:
	virtual bool findCheckType(std::vector<uint8_t> vHoldCards, std::vector<uint8_t>& vCmpCards, std::vector<uint8_t>& vResultCards ) = 0;
	static bool findCheckShun( std::vector<uint8_t> vHoldCards, std::vector<uint8_t>& vCmpCards, uint8_t nShunCnt, std::vector<uint8_t>& vResultCards )
	{
		if (vHoldCards.size() < vCmpCards.size())
		{
			return false;
		}

		std::sort(vHoldCards.begin(),vHoldCards.end());
		std::sort(vCmpCards.begin(), vCmpCards.end());
		uint8_t nMiniValue = DDZ_PARSE_VALUE(vCmpCards[0]);

		// find seqShun ;
		std::vector<uint8_t> vFindShun;
		uint8_t nLastShunValue = 0;
		for (uint8_t nIdx = 0; (nIdx + nShunCnt - 1 ) < vHoldCards.size() && (vHoldCards.size() - nIdx) >= vCmpCards.size(); )
		{
			auto nV = DDZ_PARSE_VALUE(vHoldCards[nIdx]);
			if (nV < nMiniValue)
			{
				++nIdx;
				continue;
			}

			if (DDZ_PARSE_VALUE(vHoldCards[nIdx + nShunCnt - 1 ]) != nV ) // not 3  ;
			{
				++nIdx;
				continue;
			}

			if ( nLastShunValue == nV )
			{
				++nIdx;
				continue;
			}

			if (nLastShunValue != 0 && (nLastShunValue + 1) != nV)
			{
				vFindShun.clear();
			}
			nLastShunValue = nV;

			uint8_t nC = nShunCnt;
			while (nC--)
			{
				vFindShun.push_back(vHoldCards[nIdx++]);
			}

			if (vFindShun.size() == vCmpCards.size())
			{
				break;
			}
		}

		if (vFindShun.size() < vCmpCards.size())
		{
			return false;
		}
		vResultCards = vFindShun;
		return true;
	}
	static bool pickOutGroupCardAndErase( std::vector<uint8_t>& vHoldCards ,uint8_t nValueBigThan ,uint8_t nCntPerGroup, std::vector<uint8_t>& vPickedOut )
	{
		std::sort(vHoldCards.begin(),vHoldCards.end());
		for ( uint8_t nIdx = 0; (nIdx + nCntPerGroup - 1) < vHoldCards.size() && ( vHoldCards.size() - nIdx ) >= nCntPerGroup; )
		{
			auto nV = DDZ_PARSE_VALUE(vHoldCards[nIdx]);
			if ( nV <= nValueBigThan )
			{
				++nIdx;
				continue;
			}

			if ( DDZ_PARSE_VALUE(vHoldCards[nIdx + nCntPerGroup - 1]) == nV)
			{
				auto nCnt = nCntPerGroup;
				while ( nCnt-- )
				{
					vPickedOut.push_back(vHoldCards[nIdx++]);
				}
				continue;
			}
			++nIdx;
		}
		
		// do erase 
		vHoldCards.erase(vPickedOut.begin(),vPickedOut.end());
		return vPickedOut.empty() == false;
	}
};

// find rocket 
class DDZFindRoket
	:public IDDZFindCardType
{
public:
	bool findCheckType(std::vector<uint8_t> vHoldCards, std::vector<uint8_t>& vCmpCards, std::vector<uint8_t>& vResultCards)override
	{
		auto nCnt = std::count_if(vHoldCards.begin(), vHoldCards.end(), [](uint8_t& nCard) { return DDZ_PARSE_TYPE(nCard) == ePoker_Joker; });
		if (2 != nCnt)
		{
			return false;
		}

		vResultCards.push_back(DDZ_MAKE_CARD(ePoker_Joker,15));
		vResultCards.push_back(DDZ_MAKE_CARD(ePoker_Joker, 16));
		return true;
	}
};

// find bomb
class DDZFindBomb
	:public IDDZFindCardType
{
public:
	bool findCheckType(std::vector<uint8_t> vHoldCards, std::vector<uint8_t>& vCmpCards, std::vector<uint8_t>& vResultCards)override
	{
		std::sort(vHoldCards.begin(),vHoldCards.end());
		auto nCmpValue = DDZ_PARSE_VALUE(vCmpCards.front());
		std::vector<uint8_t> vBoms;
		if ( IDDZFindCardType::pickOutGroupCardAndErase(vHoldCards, nCmpValue, 4, vBoms) == false)
		{
			return false;
		}
		vResultCards.assign(vBoms.begin(),vBoms.begin() + 3 );
		return true;
	}
};

// find 3 peices sequence
class DDZFind3PicesSeqence
	:public IDDZFindCardType
{
public:
	bool findCheckType(std::vector<uint8_t> vHoldCards, std::vector<uint8_t>& vCmpCards, std::vector<uint8_t>& vResultCards)override
	{
		// pick out 4 first 
		std::vector<uint8_t> v4CardsGroup;
		pickOutGroupCardAndErase(vHoldCards, 0, 4, v4CardsGroup );
		if ( IDDZFindCardType::findCheckShun(vHoldCards,vCmpCards,3,vResultCards) )
		{
			return true;
		}
		else
		{
			// put back 4 cards group , check again 
			if (vHoldCards.empty() == false)
			{
				vHoldCards.insert(vHoldCards.end(), v4CardsGroup.begin(), v4CardsGroup.end());

				if (IDDZFindCardType::findCheckShun(vHoldCards, vCmpCards, 3, vResultCards))
				{
					return true;
				}
			}
			
		}
		return false;
	}
};

// find pair sequence 
class DDZFindPairSequence
	:public IDDZFindCardType
{
public:
	bool findCheckType(std::vector<uint8_t> vHoldCards, std::vector<uint8_t>& vCmpCards, std::vector<uint8_t>& vResultCards)override
	{
		if (vHoldCards.size() < vCmpCards.size())
		{
			return false;
		}

		std::vector<uint8_t> v4Holds, v3Holds;
		pickOutGroupCardAndErase(vHoldCards, 0, 4, v4Holds);
		pickOutGroupCardAndErase(vHoldCards, 0, 3, v3Holds);

		if (IDDZFindCardType::findCheckShun(vHoldCards, vCmpCards, 2, vResultCards))
		{
			return true;
		}

		// push back 3 
		if (v3Holds.empty() == false)
		{
			vHoldCards.insert(vHoldCards.end(), v3Holds.begin(), v3Holds.end());
			if (IDDZFindCardType::findCheckShun(vHoldCards, vCmpCards, 2, vResultCards))
			{
				return true;
			}
		}


		// push back 4 
		if (v4Holds.empty() == false)
		{
			vHoldCards.insert(vHoldCards.end(), v4Holds.begin(), v4Holds.end());
			if (IDDZFindCardType::findCheckShun(vHoldCards, vCmpCards, 2, vResultCards))
			{
				return true;
			}
		}
		return false;
	}
};

// find single sequence 
class DDZFindSingleSequence
	:public IDDZFindCardType
{
public:
	bool findCheckType(std::vector<uint8_t> vHoldCards, std::vector<uint8_t>& vCmpCards, std::vector<uint8_t>& vResultCards)override
	{
		if (vHoldCards.size() < vCmpCards.size())
		{
			return false;
		}

		std::sort(vCmpCards.begin(),vCmpCards.end());
		auto nMinValue = DDZ_PARSE_VALUE(vCmpCards.front());
		std::vector<uint8_t> v4Holds, v3Holds , v2Holds;
		pickOutGroupCardAndErase(vHoldCards, nMinValue, 4, v4Holds);
		pickOutGroupCardAndErase(vHoldCards, nMinValue, 3, v3Holds);
		pickOutGroupCardAndErase(vHoldCards, nMinValue, 2, v2Holds);
		if (IDDZFindCardType::findCheckShun(vHoldCards, vCmpCards, 1, vResultCards))
		{
			return true;
		}

		if ( v2Holds.empty() == false )
		{
			vHoldCards.insert(vHoldCards.end(), v2Holds.begin(), v2Holds.end());
			if (IDDZFindCardType::findCheckShun(vHoldCards, vCmpCards, 1, vResultCards))
			{
				return true;
			}
		}

		if (v3Holds.empty() == false)
		{
			vHoldCards.insert(vHoldCards.end(), v3Holds.begin(), v3Holds.end());
			if (IDDZFindCardType::findCheckShun(vHoldCards, vCmpCards, 1, vResultCards))
			{
				return true;
			}
		}

		if (v4Holds.empty() == false)
		{
			vHoldCards.insert(vHoldCards.end(), v4Holds.begin(), v4Holds.end());
			if (IDDZFindCardType::findCheckShun(vHoldCards, vCmpCards, 1, vResultCards))
			{
				return true;
			}
		}

		return false;
	}
};

// find 3 pices 
class DDZFind3Pices
	:public IDDZFindCardType
{
public:
	bool findCheckType(std::vector<uint8_t> vHoldCards, std::vector<uint8_t>& vCmpCards, std::vector<uint8_t>& vResultCards)override
	{
		if (vHoldCards.size() < vCmpCards.size())
		{
			return false;
		}

		auto nMinValue = DDZ_PARSE_VALUE(vCmpCards.front());
		std::vector<uint8_t> v4Holds;
		pickOutGroupCardAndErase(vHoldCards, nMinValue, 4, v4Holds);

		if (IDDZFindCardType::findCheckShun(vHoldCards, vCmpCards, 3, vResultCards))
		{
			return true;
		}

		if (v4Holds.empty() == false)
		{
			vHoldCards.insert(vHoldCards.end(), v4Holds.begin(), v4Holds.end());
			if (IDDZFindCardType::findCheckShun(vHoldCards, vCmpCards, 3, vResultCards))
			{
				return true;
			}
		}
		return false;
	}
};


// find pair 
class DDZFindPair
	:public IDDZFindCardType
{
public:
	bool findCheckType(std::vector<uint8_t> vHoldCards, std::vector<uint8_t>& vCmpCards, std::vector<uint8_t>& vResultCards)override
	{
		if (vHoldCards.size() < vCmpCards.size())
		{
			return false;
		}

		auto nMinValue = DDZ_PARSE_VALUE(vCmpCards.front());
		std::vector<uint8_t> v4Holds, v3Holds,v2Holds;
		pickOutGroupCardAndErase(vHoldCards, nMinValue, 4, v4Holds);
		pickOutGroupCardAndErase(vHoldCards, nMinValue, 3, v3Holds);
		pickOutGroupCardAndErase(vHoldCards, nMinValue, 2, v2Holds);
		
		if (v2Holds.empty() == false)
		{
			vResultCards.push_back(v2Holds[0]);
			vResultCards.push_back(v2Holds[1]);
			return true;
		}
 
		if (v3Holds.empty() == false)
		{
			vResultCards.push_back(v3Holds[0]);
			vResultCards.push_back(v3Holds[1]);
			return true;
		}

		if (v4Holds.empty() == false)
		{
			vResultCards.push_back(v4Holds[0]);
			vResultCards.push_back(v4Holds[1]);
			return true;
		}

		return false;
	}
};

// find single 
class DDZFindSingle
	:public IDDZFindCardType
{
public:
	bool findCheckType(std::vector<uint8_t> vHoldCards, std::vector<uint8_t>& vCmpCards, std::vector<uint8_t>& vResultCards)override
	{
		auto nMinValue = DDZ_PARSE_VALUE(vCmpCards.front());
		std::vector<uint8_t> v4Holds, v3Holds,v2Holds;
		pickOutGroupCardAndErase(vHoldCards, nMinValue, 4, v4Holds);
		pickOutGroupCardAndErase(vHoldCards, nMinValue, 3, v3Holds);
		pickOutGroupCardAndErase(vHoldCards, nMinValue, 2, v2Holds);

		vHoldCards.insert(vHoldCards.end(),v2Holds.begin(),v2Holds.end());
		vHoldCards.insert(vHoldCards.end(), v3Holds.begin(), v3Holds.end());
		vHoldCards.insert(vHoldCards.end(), v4Holds.begin(), v4Holds.end());

		for ( auto& ref : vHoldCards )
		{
			if (DDZ_PARSE_VALUE(ref) > DDZ_PARSE_VALUE(vCmpCards.front()))
			{
				vResultCards.push_back(ref);
				return true;
			}
		}
		return false;
	}
};

// find aircraftWith wings 
class DDZFindAircraftWithWings
	:public IDDZFindCardType
{
public:
	bool findCheckType(std::vector<uint8_t> vHoldCards, std::vector<uint8_t>& vCmpCards, std::vector<uint8_t>& vResultCards)override
	{
		if (vHoldCards.size() < vCmpCards.size())
		{
			return false;
		}
		// find 3 seq
		std::sort(vCmpCards.begin(), vCmpCards.end());
		std::vector<uint8_t> v3Sequnc;
		for (uint8_t nIdx = 0; (nIdx + 2) < vCmpCards.size(); )
		{
			if (DDZ_PARSE_VALUE(vCmpCards[nIdx]) == DDZ_PARSE_VALUE(vCmpCards[nIdx + 2]))
			{
				uint8_t nCnt = 3;
				while (nCnt--)
				{
					v3Sequnc.push_back(vCmpCards[nIdx++]);
				}
				continue;
			}
			++nIdx;
		}

		auto nShunCnt = v3Sequnc.size() / 3;
		if ((nShunCnt * 3 + 2 * nShunCnt) != vCmpCards.size() && (nShunCnt * 3 + nShunCnt) != vCmpCards.size())
		{
			v3Sequnc.erase(v3Sequnc.begin(), v3Sequnc.end() + 2);
		}
		nShunCnt = v3Sequnc.size() / 3;
		if (0 == nShunCnt)
		{
			LOGFMTE( "why shun3 is 0 ? " );
			return false;
		}
		
		// check shun ok ?
		DDZFind3PicesSeqence t3Seq;
		std::vector<uint8_t> vFinded3Seq;
		if (false == t3Seq.findCheckType(vHoldCards, v3Sequnc, vFinded3Seq))
		{
			return false;
		}

		// erase vfinded from hold 
		vHoldCards.erase(vFinded3Seq.begin(), vFinded3Seq.end());
		vResultCards = vFinded3Seq;
		bool isFollowPair = vCmpCards.size() == nShunCnt * 3 + nShunCnt * 2;
		if ( isFollowPair )
		{
			std::vector<uint8_t> vFindPair;
			DDZFindPair ttFindP;
			while (nShunCnt-- )
			{
				vFindPair.clear();
				decltype(vFindPair) vMini = { DDZ_MAKE_CARD(ePoker_Club,0),DDZ_MAKE_CARD(ePoker_Club,0)  };
				if (ttFindP.findCheckType(vHoldCards, vMini,vFindPair) == false )
				{
					vResultCards.clear();
					return false;
				}
				vResultCards.insert(vResultCards.end(), vFindPair.begin(), vFindPair.end());
				vHoldCards.erase(vFindPair.begin(),vFindPair.end());
			}
			return true;
		}
		
		// when singe mode 
		std::vector<uint8_t> vFindSingle;
		DDZFindSingle ttFindP;
		while (nShunCnt--)
		{
			vFindSingle.clear();
			decltype(vFindSingle) vMini = { DDZ_MAKE_CARD(ePoker_Club,0) };
			if (ttFindP.findCheckType(vHoldCards, vMini, vFindSingle) == false)
			{
				vResultCards.clear();
				return false;
			}
			vResultCards.insert(vResultCards.end(), vFindSingle.begin(), vFindSingle.end());
			vHoldCards.erase(vFindSingle.begin(), vFindSingle.end());
		}
		return true;
	}
};

// find 4 with 2 
class DDZFind4Follow2
	:public IDDZFindCardType
{
public:
	bool findCheckType(std::vector<uint8_t> vHoldCards, std::vector<uint8_t>& vCmpCards, std::vector<uint8_t>& vResultCards)override
	{
		if (vHoldCards.size() < vCmpCards.size())
		{
			return false;
		}

		std::sort(vCmpCards.begin(), vCmpCards.end());
		uint8_t n4Value = 0;
		for (uint8_t nIdx = vCmpCards.size() - 1; (nIdx - 3) >= 0u; --nIdx)
		{
			if (DDZ_PARSE_VALUE(vCmpCards[nIdx]) == DDZ_PARSE_VALUE(vCmpCards[nIdx - 3]))
			{
				n4Value = vCmpCards[nIdx];
				break;
			}
		}

		if (0 == n4Value)
		{
			LOGFMTE("why do not have 4 value commponent");
			return false;
		}

		// check find 4 card 
		std::vector<uint8_t> vCheck4 = { n4Value,n4Value,n4Value,n4Value };
		std::vector<uint8_t> vFinded4;
		DDZFindBomb tBomCheck;
		if (tBomCheck.findCheckType(vHoldCards, vCheck4, vFinded4) == false) // do not have proper 4 card 
		{
			return false;
		}

		// erase vFinded4 
		vHoldCards.erase(vFinded4.begin(), vFinded4.end());
		vResultCards = vFinded4;
		
		// find follow 
		bool isFollowPair = vCmpCards.size() == 6;
		if (isFollowPair)
		{
			std::vector<uint8_t> vFindPair;
			DDZFindPair ttFindP;
			decltype(vFindPair) vMini = { DDZ_MAKE_CARD(ePoker_Club,0),DDZ_MAKE_CARD(ePoker_Club,0) }; // any pair will be ok 
			if (ttFindP.findCheckType(vHoldCards, vMini, vFindPair) == false)
			{
				vResultCards.clear();
				return false;
			}
			vResultCards.insert(vResultCards.end(), vFindPair.begin(), vFindPair.end());
			vHoldCards.erase(vFindPair.begin(), vFindPair.end());
			return true;
		}

		// when singe mode 
		std::vector<uint8_t> vFindSingle;
		DDZFindSingle ttFindP;
		decltype(vFindSingle) vMini = { DDZ_MAKE_CARD(ePoker_Club,0) };
		if (ttFindP.findCheckType(vHoldCards, vMini, vFindSingle) == false)
		{
			vResultCards.clear();
			return false;
		}
		vResultCards.insert(vResultCards.end(), vFindSingle.begin(), vFindSingle.end());
		vHoldCards.erase(vFindSingle.begin(), vFindSingle.end());
		return true;
	}
};

// find 3 follow 1 
class DDZFind3Follow1
	:public DDZFindAircraftWithWings
{
public:

};
