#pragma once
#include "NativeTypes.h"
class IPoker
{
public:
	virtual ~IPoker(){}
	virtual void init() = 0;
	virtual void shuffle() = 0;
	virtual void pushCardToFron( uint8_t nCard ) = 0;
	virtual uint8_t getLeftCardCount() = 0;
	virtual uint8_t distributeOneCard() = 0;
};