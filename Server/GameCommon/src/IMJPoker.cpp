#include "IMJPoker.h"
#include "log4z.h"
#include "ServerCommon.h"
void IMJPoker::init()
{
	m_vAllCards.clear();
	m_nCurCardIdx = 0;
	// every card are 4 count 
	for (uint8_t nCnt = 0; nCnt < 4; ++nCnt)
	{
		// add base 
		uint8_t vType[3] = { eCT_Wan,eCT_Tiao,eCT_Tong };
		for (uint8_t nType : vType)
		{
			for (uint8_t nValue = 1; nValue <= 9; ++nValue)
			{
				m_vAllCards.push_back(makeCardNumber((eMJCardType)nType, nValue));
			}
		}
	}
}

void IMJPoker::shuffle()
{
	uint16_t n = 0;
	for (uint16_t i = 0; i < m_vAllCards.size() - 2; ++i)
	{
		n = rand() % (m_vAllCards.size() - i - 1) + i + 1;
		m_vAllCards[i] = m_vAllCards[n] + m_vAllCards[i];
		m_vAllCards[n] = m_vAllCards[i] - m_vAllCards[n];
		m_vAllCards[i] = m_vAllCards[i] - m_vAllCards[n];
	}
	m_nCurCardIdx = 0;
	// set new card 
#ifdef _DEBUG
	//VEC_UINT8 vHoldCard;
	//vHoldCard.push_back(make_Card_Num(eCT_Wan,1));
	//vHoldCard.push_back(make_Card_Num(eCT_Wan, 1));
	//vHoldCard.push_back(make_Card_Num(eCT_Wan, 9));
	//vHoldCard.push_back(make_Card_Num(eCT_Wan, 9));

	//for ( uint8_t nIdx = 1; nIdx <= 9; ++nIdx )
	//{
	//	vHoldCard.push_back(make_Card_Num(eCT_Wan, nIdx ) );
	//}

	//// set new card erase old
	//for ( auto& ref : vHoldCard )
	//{
	//	auto iter = std::find(m_vAllCards.begin(),m_vAllCards.end(),ref);
	//	m_vAllCards.erase(iter);
	//}
	//vHoldCard.insert(vHoldCard.end(),m_vAllCards.begin(),m_vAllCards.end());
	//m_vAllCards.swap(vHoldCard);
#endif
	// send new 
	//debugPokerInfo();
}

void IMJPoker::pushCardToFron(uint8_t nCard)
{
	std::size_t nFindIdx = -1;
	for (std::size_t nIdx = m_nCurCardIdx; nIdx < m_vAllCards.size(); ++nIdx)
	{
		if (nCard == m_vAllCards[nIdx])
		{
			nFindIdx = nIdx;
			break;
		}
	}

	if (nFindIdx == (std::size_t) - 1 || nFindIdx == m_nCurCardIdx)
	{
		return;
	}

	m_vAllCards[nFindIdx] = m_vAllCards[m_nCurCardIdx] + m_vAllCards[nFindIdx];
	m_vAllCards[m_nCurCardIdx] = m_vAllCards[nFindIdx] - m_vAllCards[m_nCurCardIdx];
	m_vAllCards[nFindIdx] = m_vAllCards[nFindIdx] - m_vAllCards[m_nCurCardIdx];

	LOGFMTD("push card front effected card = %u", nCard);
}

uint8_t IMJPoker::getLeftCardCount()
{
	if (m_vAllCards.size() <= m_nCurCardIdx)
	{
		return 0;
	}

	return m_vAllCards.size() - m_nCurCardIdx;
}

uint8_t IMJPoker::distributeOneCard()
{
	if ( getLeftCardCount() <= 0 )
	{
		return 0;
	}

	if (m_vAllCards[m_nCurCardIdx] == 0)
	{
		++m_nCurCardIdx;
		LOGFMTE("why have a zero value ? ");
	}

	return m_vAllCards[m_nCurCardIdx++];
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
		LOGFMTD("%u �� \n", cardValue);
		break;
	case eCT_Tong:
		LOGFMTD("%u Ͳ \n", cardValue);
		break;
	case eCT_Tiao:
		LOGFMTD("%u �� \n", cardValue);
		break;
	case eCT_Feng:
		switch (cardValue)
		{
		case 1:
			LOGFMTD("�� ��\n");
			break;
		case 2:
			LOGFMTD("�� ��\n");
			break;
		case 3:
			LOGFMTD("�� ��\n");
			break;
		case 4:
			LOGFMTD("�� ��\n");
			break;
		default:
			LOGFMTD("unknown �� card = %u \n", nCard);
			break;
		}
		break;
	case eCT_Jian:
		switch (cardValue)
		{
		case 1:
			LOGFMTD("�� \n");
			break;
		case 2:
			LOGFMTD("�� \n");
			break;
		case 3:
			LOGFMTD("��\n");
			break;
		default:
			LOGFMTD("unknown ���� card = %u \n", nCard);
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