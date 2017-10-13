#pragma once
#include "IPeerCard.h"
#include "BJPoker.h"
#include "CommonDefine.h"
#include "json\json.h"
#define PEER_CARD_COUNT 3 
#define MAX_GROUP_CNT 3
#include "BJDefine.h"
class BJPlayerCard
	:public IPeerCard
{
public:
	struct stGroupCard
	{
	public:
		stGroupCard() { m_vCard.resize(PEER_CARD_COUNT); }
		void reset();
		void setCard( std::vector<uint16_t>& vCompsitCard );
		eBJCardType getType() { return m_eCardType; }
		uint32_t getWeight() { return m_nWeight; }
		uint16_t getCardByIdx( uint8_t nIdx );
	protected:
		eBJCardType m_eCardType;
		uint32_t m_nWeight;
		std::vector<uint16_t> m_vCard;  // uint16_t , ����8λ��ǰһ��8λ��ʾ�����ƣ������8λ��ʾ��ӱ�ʾ�ơ����û�������Ϣ��ǰһ��8λ����0 ��
	};
public:
	BJPlayerCard();
	void reset()override;
	void addCompositCardNum(uint8_t nCardCompositNum)override;
	const char* getNameString()override;
	uint32_t getWeight()override;
	IPeerCard* swap(IPeerCard* pTarget)override;
	uint8_t getCardByIdx(uint8_t nidx)override;
	bool getGroupInfo( uint8_t nGroupIdx , uint8_t& nGroupType , std::vector<uint16_t>& vGroupCards ); // uint16_t , ����8λ��ǰһ��8λ��ʾ�����ƣ������8λ��ʾ��ӱ�ʾ�ơ����û�������Ϣ��ǰһ��8λ����0 ��
	void setCurGroupIdx( uint8_t nGrpIdx );
	uint8_t getCurGroupIdx();
	bool setCardsGroup(std::vector<uint16_t>& vGroupedCards); // uint16_t , ����8λ��ǰһ��8λ��ʾ�����ƣ������8λ��ʾ��ӱ�ʾ�ơ����û�������Ϣ��ǰһ��8λ����0 ��
	bool holdCardToJson(Json::Value& vHoldCards);
	bool groupCardToJson(Json::Value& vHoldCards);
	// xi pai used 
	void getHoldCards( std::vector<uint8_t>& vHoldCards );
	stGroupCard& getGroupByIdx(uint8_t nGroupIdx );
	eXiPaiType getXiPaiType( bool isEnableSanQing, bool isEnableShunQingDaTou );
protected:
	std::vector<uint8_t> m_vHoldCards;
	std::vector<stGroupCard> m_vGroups;
	uint8_t m_nCurGroupIdx;
};