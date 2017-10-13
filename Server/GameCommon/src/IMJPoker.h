#pragma once
#include "IPoker.h"
#include <vector>
#include "MJDefine.h"
class IMJPoker
	:public IPoker
{
public:
	typedef std::vector<uint8_t> VEC_UINT8;
public:
	void init( Json::Value& jsOpt )override;
	static eMJCardType parseCardType(uint8_t nCardNum);
	static uint8_t parseCardValue(uint8_t nNum);
	static uint8_t makeCardNumber(eMJCardType eType, uint8_t nValue);
	static void parseCardTypeValue(uint8_t nCardNum, eMJCardType& eType, uint8_t& nValue);
	static void debugSinglCard(uint8_t nCard);
};

#ifndef CARD_TYPE_HELP
#define CARD_TYPE_HELP
#define card_Type(cardNum) IMJPoker::parseCardType((cardNum))
#define card_Value(cardNum) IMJPoker::parseCardValue((cardNum))
#define make_Card_Num(type,value) IMJPoker::makeCardNumber((type),(value))
#endif 