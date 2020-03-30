#include "CardPoker.h"
#include <assert.h>
#include <stdlib.h>

// Poker
void CGuanDanPoker::init( Json::Value& jsOpts )
{
	for (uint8_t nType = ePoker_None; nType < ePoker_NoJoker; ++nType )
	{
		for (uint8_t nValue = 2; nValue <= 14; ++nValue)  // 14 => A 
		{
			auto nCard = GD_MAKE_CARD(nType, nValue);
			addCardToPoker(nCard);
			addCardToPoker(nCard);
		}
	}

	// add wang ;
	addCardToPoker(GD_MAKE_CARD(ePoker_Joker,18));
	addCardToPoker(GD_MAKE_CARD(ePoker_Joker,18));
	addCardToPoker(GD_MAKE_CARD(ePoker_Joker,19));
	addCardToPoker(GD_MAKE_CARD(ePoker_Joker,19));

	shuffle() ;
}

void CGuanDanPoker::makeSpecialCard(std::vector<uint8_t>& vMakedCards)
{
	return;

	//vMakedCards.push_back(DDZ_MAKE_CARD(ePoker_Joker, 18));
	//vMakedCards.push_back(DDZ_MAKE_CARD(ePoker_Joker, 19));

	//vMakedCards.push_back(DDZ_MAKE_CARD(ePoker_Heart, 16));
	//vMakedCards.push_back(DDZ_MAKE_CARD(ePoker_Sword, 16));
	//vMakedCards.push_back(DDZ_MAKE_CARD(ePoker_Club, 16));
	//vMakedCards.push_back(DDZ_MAKE_CARD(ePoker_Diamond, 16));

	
	{
		for (uint8_t nValue = 2; nValue <= 14; ++nValue)  // 14 => A 
		{
			for (uint8_t nType = ePoker_None; nType < ePoker_NoJoker; ++nType)
			{
				vMakedCards.push_back(GD_MAKE_CARD(nType, nValue));
			}
			
		}
	}

	// add wang ;
	vMakedCards.push_back(GD_MAKE_CARD(ePoker_Joker, 18));
	vMakedCards.push_back(GD_MAKE_CARD(ePoker_Joker, 19));
}

void CGuanDanPoker::initAllCardWithCards(std::vector<uint8_t> vCards) {
	if (vCards.size() == m_vCards.size()) {
		m_vCards.clear();
		m_vCards.assign(vCards.begin(), vCards.end());
	}
	else {
		assert(false, "invalid not shuffle poke card amount");
	}
}
