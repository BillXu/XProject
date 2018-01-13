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
	return;
	//	ºÚÌÒA K Q 10 9 8 Ã·»¨8 ºìÌÒ8 ·½¿é8
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Heart, 1));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Heart, 2));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Heart, 3));

	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Heart, 4));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Heart, 5));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Heart, 6));

	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Heart, 7));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Heart, 8));
	vMakedCards.push_back(TT_MAKE_CARD(ePoker_Heart, 9));

}

