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
	unsigned char GetCardFaceNum(bool bSpecailA = false) const { if (bSpecailA && m_nCardFaceNum == 1)return 14; return m_nCardFaceNum; } // face num is the num show on the card ;
	unsigned char GetCardCompositeNum() const;  // Composite are make of face num and card type ;
	eCardType GetType(){ return m_eType ;}
	void LogCardInfo();
	CCard& SetCard(eCardType etype, unsigned char nFaceNum );
	bool isJoker() { return GetCardFaceNum() > 52; }
protected:
	eCardType m_eType ;
	unsigned char m_nCardFaceNum ;
};

class CGoldenPoker
	: public IPoker
{
public:
	void init(Json::Value& jsOpts)override;
	void makeSpecialCard(std::vector<uint8_t>& vMakedCards)override;
protected:
	void LogCardInfo();
};