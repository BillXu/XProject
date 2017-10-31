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
		addCardToPoker(BJ_MAKE_CARD(ePoker_Joker, 14));
		addCardToPoker(BJ_MAKE_CARD(ePoker_Joker, 15));
	}

	shuffle();
}

