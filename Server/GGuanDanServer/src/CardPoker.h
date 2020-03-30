#pragma once 
#include "IPoker.h"
#include "log4z.h"
#include "PokerDefine.h"
class CGuanDanPoker
	: public IPoker
{
public:
	void init(Json::Value& jsOpts)override;
	void makeSpecialCard(std::vector<uint8_t>& vMakedCards)override;
	void initAllCardWithCards(std::vector<uint8_t> vCards);
};

#define GD_MAKE_CARD( type, value ) IPoker::makeCard(value,type )
#define GD_PARSE_VALUE( cardNum ) IPoker::parsePartA(cardNum)
#define GD_PARSE_TYPE( cardNum ) IPoker::parsePartB(cardNum)