#pragma once 
#include "IPoker.h"
#include "PokerDefine.h"

class CBJPoker
	: public IPoker
{
public:
	void init(Json::Value& jsOpts)override;
	void makeSpecialCard(std::vector<uint8_t>& vMakedCards)override;
};

#define BJ_MAKE_CARD( type, value ) IPoker::makeCard(value,type )
#define BJ_PARSE_VALUE( cardNum ) IPoker::parsePartA(cardNum)
#define BJ_PARSE_TYPE( cardNum ) IPoker::parsePartB(cardNum)