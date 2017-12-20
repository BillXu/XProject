#include "CardPoker.h"
#include <assert.h>
#include <stdlib.h>

// Poker
void CDouDiZhuPoker::init( Json::Value& jsOpts )
{
	for (uint8_t nType = ePoker_None; nType < ePoker_NoJoker; ++nType )
	{
		for (uint8_t nValue = 3; nValue <= 14; ++nValue)  // 14 => A 
		{
			addCardToPoker(DDZ_MAKE_CARD(nType,nValue));
		}

		addCardToPoker(DDZ_MAKE_CARD(nType, 16));  // 2 ;
	}

	// add wang ;
	addCardToPoker(DDZ_MAKE_CARD(ePoker_Joker,18));
	addCardToPoker(DDZ_MAKE_CARD(ePoker_Joker, 19));

	shuffle() ;
}

void CDouDiZhuPoker::makeSpecialCard(std::vector<uint8_t>& vMakedCards)
{
	//vMakedCards.push_back(DDZ_MAKE_CARD(ePoker_Joker, 18));
	//vMakedCards.push_back(DDZ_MAKE_CARD(ePoker_Joker, 19));

	//vMakedCards.push_back(DDZ_MAKE_CARD(ePoker_Heart, 16));
	//vMakedCards.push_back(DDZ_MAKE_CARD(ePoker_Sword, 16));
	//vMakedCards.push_back(DDZ_MAKE_CARD(ePoker_Club, 16));
	//vMakedCards.push_back(DDZ_MAKE_CARD(ePoker_Diamond, 16));

	
	{
		for (uint8_t nValue = 3; nValue <= 14; ++nValue)  // 14 => A 
		{
			for (uint8_t nType = ePoker_None; nType < ePoker_NoJoker; ++nType)
			{
				vMakedCards.push_back(DDZ_MAKE_CARD(nType, nValue));
			}
			
		}

		for (uint8_t nType = ePoker_None; nType < ePoker_NoJoker; ++nType)
			vMakedCards.push_back(DDZ_MAKE_CARD(nType, 16));  // 2 ;
	}

	// add wang ;
	vMakedCards.push_back(DDZ_MAKE_CARD(ePoker_Joker, 18));
	vMakedCards.push_back(DDZ_MAKE_CARD(ePoker_Joker, 19));
}
