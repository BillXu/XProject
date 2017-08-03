#pragma once 
#include "IPoker.h"
class CCard
{
public:
	enum eCardType
	{
		eCard_None,
		eCard_Diamond = eCard_None, // fangkuai
        eCard_Club, // cao hua
		eCard_Heart, // hong tao
		eCard_Sword, // hei tao 
		eCard_NoJoker,
		eCard_Joker = eCard_NoJoker, // xiao wang
		eCard_BigJoker, // da wang
		eCard_Max,
	};
public:
	CCard( unsigned char nCompositeNum =2 );
	~CCard();
	void RsetCardByCompositeNum( unsigned char nCompositeNum );
	unsigned char GetCardFaceNum( bool bSpecailA = false ){ if ( bSpecailA && m_nCardFaceNum == 1 )return 14 ;return m_nCardFaceNum ; } // face num is the num show on the card ;
	unsigned char GetCardCompositeNum();  // Composite are make of face num and card type ;
	eCardType GetType(){ return m_eType ;}
	void LogCardInfo();
	CCard& SetCard(eCardType etype, unsigned char nFaceNum );
protected:
	eCardType m_eType ;
	unsigned char m_nCardFaceNum ;
};

class CPoker
	: public IPoker
{
public:
	CPoker();
	~CPoker();
	void init()override;
	void shuffle()override;
	void pushCardToFron(uint8_t nCard)override;
	uint8_t getLeftCardCount()override { return m_nCardCount - m_nCurIdx; };
	uint8_t distributeOneCard()override;
	unsigned short GetAllCard(){ return m_nCardCount;  }
	unsigned char getCardNum( unsigned char nIdx );
protected:
	void AddCard(unsigned char nCard );   // invoke when init
	void AddCard(CCard* pcard);   // invoke when init
	void SetupCardCount(unsigned short nCount );
	void LogCardInfo();
protected:
	unsigned char* m_vCards ;
	unsigned short m_nCardCount ;
	unsigned short m_nCurIdx ;
};