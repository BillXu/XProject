#include "CardPoker.h"
#include <assert.h>
#include <stdlib.h>
#ifdef SERVER
#include "log4z.h"
#endif
// card 
CCard::CCard(  unsigned char nCompositeNum  )
	:m_eType(eCard_Heart)
	,m_nCardFaceNum(0)
{
	RsetCardByCompositeNum(nCompositeNum) ;
}

CCard::~CCard()
{
	//printf("delete this card %u , addr = %u \n",m_nCardFaceNum,(unsigned int)this) ;
}

void CCard::RsetCardByCompositeNum( unsigned char nCompositeNum )
{
	assert(nCompositeNum > 0 && nCompositeNum <= 54 && "illegal composite Number" );
	if ( (nCompositeNum > 0 && nCompositeNum <= 54) != true )
	{
#ifdef SERVER
		LOGFMTE("Illegal composite Number = %d",nCompositeNum );
#endif
		return ;
	}

	if ( nCompositeNum == 53 ) 
	{
		m_eType = eCard_Joker ;
		m_nCardFaceNum = 53 ;
	}
	else if ( nCompositeNum == 54 )
	{
		m_eType = eCard_BigJoker ;
		m_nCardFaceNum = 54 ;
	}
	else
	{
		m_eType = (eCardType)((nCompositeNum - 1)/13);
		m_nCardFaceNum = nCompositeNum - m_eType * 13 ;
	}
}

unsigned char CCard::GetCardCompositeNum()
{
	return ( m_eType * 13 + m_nCardFaceNum );
}

CCard& CCard::SetCard(eCardType etype, unsigned char nFaceNum )
{
	if ( etype < eCard_None || etype >= eCard_Max )
	{
        #ifdef SERVER
		LOGFMTE("unknown card type =%d", etype ) ;
#endif
		return *this;
	}

	if ( nFaceNum <=0 ||nFaceNum >54 )
	{
        #ifdef SERVER
		LOGFMTE("unlegal face number = %d",nFaceNum ) ;
#endif
	}
	m_eType = etype ;
	m_nCardFaceNum = nFaceNum ;
	return *this;
}

void CCard::LogCardInfo()
{
	const char* pType = NULL ;
	switch ( m_eType )
	{
	case eCard_Heart:
		pType = "hong tao ";
		break;
	case eCard_Sword:
		pType = "hei tao";
		break;
	case eCard_Club:
		pType = "cao hua";
		break;
	case eCard_Diamond:
		pType = "fang kuai";
		break;
	default:
        pType = "unknown";
		break; 
	}
    #ifdef SERVER
	LOGFMTD("this is %s : %d . Composite Number: %d",pType,m_nCardFaceNum, GetCardCompositeNum() );
#endif
}

// Poker
CPoker::CPoker()
{
	m_vCards = NULL ;
	m_nCurIdx = 0 ;
	m_nCardCount = 0 ;
}

CPoker::~CPoker()
{
	if (m_vCards == NULL )
	{

	}
	else
	{
		delete [] m_vCards ;
	}
} 


void CPoker::init()
{
	SetupCardCount(52);
	for ( int i = 1 ; i <= 52 ; ++i )
	{
		AddCard(i) ;
	}
	shuffle() ;
}

void CPoker::pushCardToFron(uint8_t nCard)
{
	LOGFMTE( "not implement this method  CPoker::pushCardToFron" );
}

uint8_t CPoker::distributeOneCard()
{
	if ( m_nCardCount <= m_nCurIdx )
	{
		shuffle();
	}
	return m_vCards[m_nCurIdx++] ;
}

unsigned char CPoker::getCardNum( unsigned char nIdx )
{
	if ( m_nCardCount <= nIdx )
	{
		return 0 ;
	}
	return m_vCards[nIdx] ;
}

void CPoker::shuffle()
{
	// ---
	int n = 0 ;
	for ( int i = 0 ; i < m_nCardCount ; ++i )
	{
		n = rand() % m_nCardCount  ;
		if ( n == i )
		{
			continue;
		}
		m_vCards[i] = m_vCards[n] + m_vCards[i] ;
		m_vCards[n] = m_vCards[i] - m_vCards[n] ;
		m_vCards[i] = m_vCards[i] - m_vCards[n] ;
 	}
	m_nCurIdx = 0 ;
	//LogCardInfo();
}

void CPoker::AddCard(unsigned char nCard )   // invoke when init
{
	if ( nCard < 0 || nCard > 54 )
	{
#ifdef SERVER
		LOGFMTE("unlegal card composite number = %d", nCard ) ;
#endif
		return ;
	}
	
	if ( m_nCurIdx >= m_nCardCount )
	{
#ifdef SERVER
		LOGFMTE("Poker room space is full , can not add more card ") ;
#endif
	}
	m_vCards[m_nCurIdx] = nCard ;
	++m_nCurIdx ;
}

void CPoker::AddCard(CCard* pcard)   // invoke when init
{
	AddCard(pcard->GetCardCompositeNum()) ;
}

void CPoker::SetupCardCount(unsigned short nCount )
{
	m_vCards = new unsigned char [nCount] ;
	m_nCurIdx = 0 ;
	m_nCardCount = nCount ;
}

void CPoker::LogCardInfo()
{
#ifdef SERVER
	LOGFMTD("œ¥≈∆Ω·π˚»Áœ¬£∫   ");
#endif
	CCard stCard(2);
	for ( int i = 0 ; i < m_nCardCount ; ++i )
	{
		stCard.RsetCardByCompositeNum(m_vCards[i]) ;
		stCard.LogCardInfo() ;
	}
}
