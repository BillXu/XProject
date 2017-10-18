#pragma once 
#include "IPoker.h"
#include "log4z.h"
enum ePokerType
{
	ePoker_None,
	ePoker_Diamond = ePoker_None, // fangkuai
	ePoker_Club, // cao hua
	ePoker_Heart, // hong tao
	ePoker_Sword, // hei tao 
	ePoker_NoJoker,
	ePoker_Joker = ePoker_NoJoker,
	ePoker_Max,
};

class CDouDiZhuPoker
	: public IPoker
{
public:
	void init(Json::Value& jsOpts)override;
	static uint8_t makeCard( uint8_t nType, uint8_t nValue)
	{
		if (nType >= ePoker_Max)
		{
			LOGFMTE("invalid poker type can not make poker = %u",nType );
			return 0;
		}
		uint8_t nCard = (nValue << 4 | (nType) );
		return nCard;
	}

	static uint8_t parseValue(uint8_t nCardNum)
	{
		return (nCardNum & 0xf0) >> 4 ;
	}

	static uint8_t parseType( uint8_t nCardNum )
	{
		return (nCardNum & 0x0f);
	}
};

#define POKER_MAKE_CARD( type, value ) CDouDiZhuPoker::makeCard(type,value )
#define POKER_PARSE_VALUE( cardNum ) CDouDiZhuPoker::parseValue(cardNum)
#define POKER_PARSE_TYPE( cardNum ) CDouDiZhuPoker::parseType(cardNum)