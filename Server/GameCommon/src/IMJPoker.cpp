#include "IMJPoker.h"
#include "log4z.h"
#include "ServerCommon.h"
void IMJPoker::init(Json::Value& jsOpt)
{
	// every card are 4 count 
	for (uint8_t nCnt = 0; nCnt < 4; ++nCnt)
	{
		// add base 
		uint8_t vType[3] = { eCT_Wan,eCT_Tiao,eCT_Tong };
		for (uint8_t nType : vType)
		{
			for (uint8_t nValue = 1; nValue <= 9; ++nValue)
			{
				addCardToPoker(makeCardNumber((eMJCardType)nType, nValue));
			}
		}
	}
}

eMJCardType IMJPoker::parseCardType(uint8_t nCardNum)
{
	uint8_t nType = nCardNum & 0xF0;
	nType = nType >> 4;
	if ((nType < eCT_Max && nType > eCT_None) == false)
	{
		LOGFMTE("parse card type error , cardnum = %u", nCardNum);
	}
	Assert(nType < eCT_Max && nType > eCT_None, "invalid card type");
	return (eMJCardType)nType;
}

uint8_t IMJPoker::parseCardValue(uint8_t nCardNum)
{
	return  (nCardNum & 0xF);
}

uint8_t IMJPoker::makeCardNumber(eMJCardType eType, uint8_t nValue)
{
	if (((eType < eCT_Max && eType > eCT_None) == false) || (nValue <= 9 && nValue >= 1) == false)
	{
		LOGFMTE("makeCardNumber card type error , type  = %u, value = %u ", eType, nValue);
	}

	Assert(eType < eCT_Max && eType > eCT_None, "invalid card type");
	Assert(nValue <= 10 && nValue >= 1, "invalid card value");
	uint8_t nType = eType;
	nType = nType << 4;
	uint8_t nNum = nType | nValue;
	return nNum;
}

void IMJPoker::parseCardTypeValue(uint8_t nCardNum, eMJCardType& eType, uint8_t& nValue)
{
	eType = parseCardType(nCardNum);
	nValue = parseCardValue(nCardNum);

	if (((eType < eCT_Max && eType > eCT_None) == false) || (nValue <= 9 && nValue >= 1) == false)
	{
		LOGFMTE("parseCardTypeValue card type error , type  = %u, value = %u number = %u", eType, nValue, nCardNum);
	}

	Assert(eType < eCT_Max && eType > eCT_None, "invalid card type");
	Assert(nValue <= 9 && nValue >= 1, "invalid card value");
}

void IMJPoker::debugSinglCard(uint8_t nCard)
{
	auto cardType = parseCardType(nCard);
	auto cardValue = parseCardValue(nCard);

	switch (cardType)
	{
	case eCT_None:
		break;
	case eCT_Wan:
		LOGFMTD("%u 万 \n", cardValue);
		break;
	case eCT_Tong:
		LOGFMTD("%u 筒 \n", cardValue);
		break;
	case eCT_Tiao:
		LOGFMTD("%u 条 \n", cardValue);
		break;
	case eCT_Feng:
		switch (cardValue)
		{
		case 1:
			LOGFMTD("东 风\n");
			break;
		case 2:
			LOGFMTD("南 风\n");
			break;
		case 3:
			LOGFMTD("西 风\n");
			break;
		case 4:
			LOGFMTD("北 风\n");
			break;
		default:
			LOGFMTD("unknown 风 card = %u \n", nCard);
			break;
		}
		break;
	case eCT_Jian:
		switch (cardValue)
		{
		case 1:
			LOGFMTD("中 \n");
			break;
		case 2:
			LOGFMTD("发 \n");
			break;
		case 3:
			LOGFMTD("白\n");
			break;
		default:
			LOGFMTD("unknown 箭牌 card = %u \n", nCard);
			break;
		}
		break;
	case eCT_Max:
		LOGFMTD("unknown card = %u \n", nCard);
		break;
	default:
		break;
	}
}