#pragma once
#include "MJPlayerCard.h"
class NJPlayerCard
	:public MJPlayerCard
{
public:
	bool isChued4Feng();
	bool isChued4Card( uint8_t nCard );
	uint8_t getSongGangIdx();
	void setSongGangIdx( int8_t nIdx );
	uint8_t getInvokerPengIdx( uint8_t nCard );
};