#pragma once 
#include "IPoker.h"
#include "PokerDefine.h"

class ThirteenPoker
	: public IPoker
{
public:
	void init(Json::Value& jsOpts)override;
	void makeSpecialCard(std::vector<uint8_t>& vMakedCards)override;
};

#define TT_MAKE_CARD( type, value ) IPoker::makeCard(value,type)
#define TT_PARSE_VALUE( cardNum ) IPoker::parsePartA(cardNum)
#define TT_PARSE_TYPE( cardNum ) IPoker::parsePartB(cardNum)