#include "CardPoker.h"
#include <assert.h>
#include <stdlib.h>

// Poker
void CDouDiZhuPoker::init( Json::Value& jsOpts )
{
	for (uint8_t nType = ePoker_None; nType < ePoker_NoJoker; ++nType )
	{
		for (uint8_t nValue = 1; nValue <= 13; ++nValue)
		{
			addCardToPoker(DDZ_MAKE_CARD(nType,nValue));
		}
	}

	// add wang ;
	addCardToPoker(DDZ_MAKE_CARD(ePoker_Joker,14));
	addCardToPoker(DDZ_MAKE_CARD(ePoker_Joker, 15));

	shuffle() ;
}
