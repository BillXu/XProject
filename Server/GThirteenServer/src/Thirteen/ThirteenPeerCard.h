#pragma once
#include "IPeerCard.h"
#include "ThirteenPoker.h"
#include "CommonDefine.h"
#include "json\json.h"
class ThirteenPeerCard
	:public IPeerCard
{
public:
	struct stGroupCard
	{
	public:
		stGroupCard();
		void reset();
		bool setCard(VEC_CARD& vCompsitCard);
		ThirteenType getType();
		uint32_t getWeight();
		bool holdCardToJson(Json::Value& js);
		uint8_t getCardCnt() { return m_vCard.size(); }
		uint8_t getCardByIdx(uint8_t nIdx);
	protected:
		bool isCaculated();
		void caculateCards();
	protected:
		ThirteenType m_eCardType;
		uint32_t m_nWeight;
		VEC_CARD m_vCard;
	};
	typedef std::vector<stGroupCard> VEC_GROUP;
public:
    ThirteenPeerCard();
	void setOpts();
	void addCompositCardNum( uint8_t nCardCompositNum ) override;
	bool setDao(uint8_t nIdx, VEC_CARD vCards) override;
	bool autoSetDao()override;
	bool reSetDao();
	const char*  getNameString() override;
	uint32_t getWeight() override;
	uint8_t getType()override;
	uint32_t getWeight(uint8_t nDao);
	uint8_t getType(uint8_t nDao);
	void reset() override ;
	PK_RESULT pk(IPeerCard* pTarget) override;
	void setCurGroupIdx(uint8_t nGrpIdx);
	bool getGroupInfo(uint8_t nGroupIdx, uint8_t& nGroupType, std::vector<uint8_t>& vGroupCards);
    uint8_t getCurGroupIdx(){ return m_nCurGroupIdx; }
	uint8_t getHoldCardCnt() { return m_vHoldCards.size(); }
	bool holdCardToJson(Json::Value& js);
	bool groupCardToJson(Json::Value& js);
	bool groupCardTypeToJson(Json::Value& js);
	bool groupCardWeightToJson(Json::Value& js);
	void getHoldCards(std::vector<uint8_t>& vHoldCards);

protected:
	bool autoSetDao(VEC_CARD& vCards, uint8_t nIdx);
	bool set5Tong(VEC_CARD& vCards, uint8_t nIdx);
	bool setStraightFlush(VEC_CARD& vCards, uint8_t nIdx);
	bool setTieZhi(VEC_CARD& vCards, uint8_t nIdx);
	bool setFuLu(VEC_CARD& vCards, uint8_t nIdx);
	bool setFlush(VEC_CARD& vCards, uint8_t nIdx);
	bool setStraight(VEC_CARD& vCards, uint8_t nIdx);
	bool setThreeCards(VEC_CARD& vCards, uint8_t nIdx);
	bool setDoubleDouble(VEC_CARD& vCards, uint8_t nIdx);
	bool setDouble(VEC_CARD& vCards, uint8_t nIdx);
	bool setSingle(VEC_CARD& vCards, uint8_t nIdx);

	bool setCommonStraight(VEC_CARD vCards, VEC_CARD& vTemp, uint8_t nJokerCnt, uint8_t& needJokerCnt, uint8_t& nBigValue);
	bool set12345(VEC_CARD vCards, VEC_CARD& vTemp, uint8_t nJokerCnt, uint8_t& needJokerCnt);
	bool eraseJoker(VEC_CARD& vCards, uint8_t nCnt);

protected:
	VEC_CARD m_vHoldCards;
	VEC_GROUP m_vGroups;
	uint8_t m_nCurGroupIdx;
};