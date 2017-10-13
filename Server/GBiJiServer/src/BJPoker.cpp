#include "BJPoker.h"
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
void CBJPoker::init( Json::Value& jsOpts )
{
	uint8_t nCardCnt = 52;
	if (jsOpts["baiDian"].isNull() == false && jsOpts["baiDian"].asUInt() == 1)
	{
		nCardCnt = 54;
		LOGFMTD("create bai bian niu niu ");
	}

	for ( int i = 1 ; i <= nCardCnt; ++i )
	{
		addCardToPoker(i) ;
	}
	shuffle() ;
}

void CBJPoker::LogCardInfo()
{
#ifdef SERVER
	LOGFMTD("œ¥≈∆Ω·π˚»Áœ¬£∫   ");
#endif
	CCard stCard(2);
	for ( int i = 0 ; i < m_vCards.size() ; ++i )
	{
		stCard.RsetCardByCompositeNum(m_vCards[i]) ;
		stCard.LogCardInfo() ;
	}
}
