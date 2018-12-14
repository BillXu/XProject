#pragma once
#include "NativeTypes.h"
#include "json\json.h"
#include "DouDiZhuDefine.h"
class DDZPlayerCard
{
public:
	void reset();
	void addHoldCard(uint8_t nCard);
	bool onChuCard(std::vector<uint8_t>& vCards, eFALGroupCardType nType);
	void holdCardToJson( Json::Value& jsHoldCard );
	bool lastChuToJson(Json::Value& jsHoldCard);
	bool getTuoGuanChuCards(eFALGroupCardType& nCurAndOutType, std::vector<uint8_t>& vCmpCards, std::vector<uint8_t>& vResultCards );
	void clearLastChu();
	void getHoldCard(std::vector<uint8_t>& vHoldCards);
	uint8_t getHoldCardCount();
	uint16_t getChuedCardTimes();
	bool isHaveCard( uint8_t nCard );
protected:
	std::vector<uint8_t> m_vHoldCards;
	std::vector<uint8_t> m_vLastChu;
	eFALGroupCardType m_nLastChuType;
	uint16_t m_nChuedCardTimes;
};