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
		addCardToPoker(BJ_MAKE_CARD(ePoker_Joker, 15));  // black 
		addCardToPoker(BJ_MAKE_CARD(ePoker_Joker, 16));  // red ;
	}

	shuffle();
}

void CBJPoker::makeSpecialCard(std::vector<uint8_t>& vMakedCards)
{
	//return;
	//	ºÚÌÒA K Q 10 9 8 Ã·»¨8 ºìÌÒ8 ·½¿é8
	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Heart, 1));
	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Diamond, 3));
	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Sword, 3));

	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Heart, 2));
	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Heart, 2));
	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Heart, 2));

	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Sword, 8));
	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Sword, 8));
	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Sword, 8));


	//	ºÚÌÒA K Q 10 9 8 Ã·»¨8 ºìÌÒ8 ·½¿é8
	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Club, 1));
	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Club, 3));
	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Heart, 3));

	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Heart, 4));
	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Heart, 4));
	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Heart, 4));

	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Sword, 6));
	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Sword, 6));
	vMakedCards.push_back(BJ_MAKE_CARD(ePoker_Sword, 6));
}

