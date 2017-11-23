#include "BJPoker.h"
#include <assert.h>
#include <stdlib.h>
#include "log4z.h"
// Poker
void CBJPoker::init( Json::Value& jsOpts )
{
	for (uint8_t nType = ePoker_None; nType < ePoker_NoJoker; ++nType)
	{
		for (uint8_t nValue = 1; nValue <= 13; ++nValue)
		{
			addCardToPoker(BJ_MAKE_CARD(nType, nValue));
		}
	}

	// add wang ;
	if (jsOpts["isJoker"].isNull() == false && jsOpts["isJoker"].asUInt() == 1)
	{
		addCardToPoker(BJ_MAKE_CARD(ePoker_Joker, 15));
		addCardToPoker(BJ_MAKE_CARD(ePoker_Joker, 16));
	}

	shuffle();
}

void CBJPoker::makeSpecialCard(std::vector<uint8_t>& vMakedCards)
{
	//	����A K Q 10 9 8 ÷��8 ����8 ����8
	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Heart, 1));
	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Sword, 12));
	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Heart, 13));

	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Sword, 6));
	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Club, 7));
	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Sword, 8));

	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Club, 10));
	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Sword, 11));
	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Club, 9));
 
}

