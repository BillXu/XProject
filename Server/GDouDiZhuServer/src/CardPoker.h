#pragma once 
#include "IPoker.h"
#include "log4z.h"
#include "PokerDefine.h"
class CDouDiZhuPoker
	: public IPoker
{
public:
	void init(Json::Value& jsOpts)override;
};

#define DDZ_MAKE_CARD( type, value ) IPoker::makeCard(value,type )
#define DDZ_PARSE_VALUE( cardNum ) IPoker::parsePartA(cardNum)
#define DDZ_PARSE_TYPE( cardNum ) IPoker::parsePartB(cardNum)