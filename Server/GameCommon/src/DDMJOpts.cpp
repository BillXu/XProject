#include "DDMJOpts.h"
#include "log4z.h"
#include "CommonDefine.h"
void DDMJOpts::initRoomOpts(Json::Value& jsOpts) {
	IMJOpts::initRoomOpts(jsOpts);

	m_bRandomChangeSeat = jsOpts["rcs"].asBool();
	m_bDianPaoOnePay = jsOpts["dpOnePay"].asBool();

	m_nFanLimit = 0;
	if (jsOpts["fanLimit"].isUInt()) {
		m_nFanLimit = jsOpts["fanLimit"].asUInt();
	}
	m_nGuang = 0;
	if (jsOpts["guang"].isUInt()) {
		m_nGuang = jsOpts["guang"].asUInt();
	}
}

uint16_t DDMJOpts::calculateDiamondNeed() {
#ifdef _DEBUG
	return 0;
#endif // _DEBUG
	auto nLevel = getRoundLevel();
	if (nLevel > 1)
	{
		LOGFMTE("invalid room level for game = %u , level = %u", getGameType(), nLevel);
		nLevel = 1;
	}

	// is aa true ;
	if (isAA() || getPayType() == ePayType_AA)
	{
		uint16_t vAA[] = { 1 , 2 };
		return vAA[nLevel];
	}

	uint16_t vFangZhu[] = { 4 , 8 };
	return vFangZhu[nLevel];
}

uint8_t DDMJOpts::calculateInitRound() {
#ifdef _DEBUG
	//return 2;
#endif // _DEBUG
	auto nLevel = getRoundLevel();

	if (nLevel > 1) {
		return 1;
	}
	uint8_t vRounds[2] = { 8, 16 };
	return vRounds[nLevel];
}