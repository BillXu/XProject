#include "ThirteenPoker.h"
#include <assert.h>
#include <stdlib.h>
#include "log4z.h"
// Poker
void ThirteenPoker::init(Json::Value& jsOpts)
{
	for (uint8_t nType = ePoker_None; nType < ePoker_NoJoker; ++nType)
	{
		for (uint8_t nValue = 2; nValue <= 14; ++nValue)
		{
			addCardToPoker(TT_MAKE_CARD(nType, nValue));
		}
	}

	// add wang ;
	if (jsOpts["isJoker"].isNull() == false && jsOpts["isJoker"].asUInt() == 1)
	{
		addCardToPoker(TT_MAKE_CARD(ePoker_Joker, 15));  // black 
		addCardToPoker(TT_MAKE_CARD(ePoker_Joker, 16));  // red ;
	}

	shuffle();
}

void ThirteenPoker::makeSpecialCard(std::vector<uint8_t>& vMakedCards)
{
	//return;
	//	ºÚÌÒA K Q 10 9 8 Ã·»¨8 ºìÌÒ8 ·½¿é8
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Heart, 14));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Club, 2));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Heart, 3));

	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Club, 4));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Heart, 5));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Club, 6));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Heart, 7));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Club, 8));

	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Heart, 9));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Club, 10));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Heart, 11));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Club, 12));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Heart, 13));

	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Sword, 14));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Diamond, 2));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Sword, 3));

	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Diamond, 4));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Sword, 5));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Diamond, 6));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Sword, 7));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Diamond, 8));

	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Sword, 9));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Diamond, 10));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Sword, 11));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Diamond, 12));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Sword, 13));

}

