#pragma once
#include "NativeTypes.h"
#include "json\json.h"
#include "GuanDanDefine.h"
class GDPlayerCard
{
public:
	void reset();
	void addHoldCard(uint8_t nCard);
	bool onChuCard(std::vector<uint8_t>& vCards, GD_Type nType);
	void holdCardToJson( Json::Value& jsHoldCard );
	bool lastChuToJson(Json::Value& jsHoldCard);
	//bool getTuoGuanChuCards(GD_Type& nCurAndOutType, std::vector<uint8_t>& vCmpCards, std::vector<uint8_t>& vResultCards );
	void clearLastChu();
	void getHoldCard(std::vector<uint8_t>& vHoldCards);
	uint8_t getHoldCardCount();
	uint16_t getChuedCardTimes();
	bool isHaveCard( uint8_t nCard );

	uint8_t autoGetPayTributeCard(uint8_t nDaJi);
	uint8_t autoGetBackTributeCard(uint8_t nDaJi);
	bool isCardInvalidPayTribute(uint8_t nCard, uint8_t nDaJi);
	bool isCardInvalidBackTribute(uint8_t nCard, uint8_t nDaJi);
	bool onPayCard(uint8_t nCard);

protected:
	uint8_t getBiggestCardExcept(uint8_t nDaJi);
	uint8_t getSmallestCardExcept(uint8_t nDaJi);

protected:
	std::vector<uint8_t> m_vHoldCards;
	std::vector<uint8_t> m_vLastChu;
	GD_Type m_nLastChuType;
	uint16_t m_nChuedCardTimes;
};