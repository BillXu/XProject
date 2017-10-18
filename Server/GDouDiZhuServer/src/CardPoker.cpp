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
			addCardToPoker(makeCard(nType,nValue));
		}
	}

	// add wang ;
	addCardToPoker(makeCard(ePoker_Joker,15));
	addCardToPoker(makeCard(ePoker_Joker, 16));

	shuffle() ;
}
