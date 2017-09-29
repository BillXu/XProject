#pragma once
#include "IPeerCard.h"
#include "CardPoker.h"
#include "CommonDefine.h"
#include "json\json.h"
#define PEER_CARD_COUNT 3 
#define MAX_GROUP_CNT 3
class BJPlayerCard
	:public IPeerCard
{
public:
	struct stGroupCard
	{
	public:
		enum ePeerCardType
		{
			ePeerCard_None,  // common 
			ePeerCard_Pair,  // dui zi 
			ePeerCard_Sequence, // shun zi
			ePeerCard_SameColor,  // jin hua
			ePeerCard_SameColorSequence, // shun jin
			ePeerCard_Bomb,  // bao zi 
			ePeerCard_Max,
		};
	public:
		stGroupCard() { m_vCard.resize(PEER_CARD_COUNT); }
		void reset();
		void setCard( uint8_t nA , uint8_t nB , uint8_t nC );
		ePeerCardType GetType() { return m_eCardType; }
		int8_t PKPeerCard(stGroupCard* pPeerCard);  // 1 win , 0 same , -1 failed 
		uint32_t getWeight();
		uint8_t getCardByIdx( uint8_t nIdx );
	protected:
		void arrangeCard();
	protected:
		ePeerCardType m_eCardType;
		std::vector<CCard> m_vCard;
		uint8_t m_pPairCardNum;  // used when pair ;
	};
public:
	BJPlayerCard();
	void reset()override;
	void addCompositCardNum(uint8_t nCardCompositNum)override;
	const char* getNameString()override;
	uint32_t getWeight()override;
	IPeerCard* swap(IPeerCard* pTarget)override;
	uint8_t getCardByIdx(uint8_t nidx)override;
	bool getGroupInfo( uint8_t nGroupIdx , uint8_t& nGroupType , std::vector<uint8_t>& vGroupCards );
	void setCurGroupIdx( uint8_t nGrpIdx );
	uint8_t getCurGroupIdx();
	bool setCardsGroup(std::vector<uint8_t>& vGroupedCards);
	bool holdCardToJson(Json::Value& vHoldCards);
protected:
	std::vector<uint8_t> m_vHoldCards;
	std::vector<stGroupCard> m_vGroups;
	uint8_t m_nCurGroupIdx;
};