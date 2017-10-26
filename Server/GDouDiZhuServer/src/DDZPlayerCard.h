#pragma once
#include "NativeTypes.h"
#include "json\json.h"
class DDZPlayerCard
{
public:
	void reset();
	void addHoldCard(uint8_t nCard);
	bool onChuCard(std::vector<uint8_t>& vCards );
	void holdCardToJson( Json::Value& jsHoldCard );
	void clearLastChu();
	uint8_t getHoldCardCount();
	uint16_t getChuedCardTimes();
protected:
	std::vector<uint8_t> m_vHoldCards;
	std::vector<uint8_t> m_vLastChu;
	uint16_t m_nChuedCardTimes;
};