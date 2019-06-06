#include "FXMJOpts.h"
#include "log4z.h"
#include "CommonDefine.h"
void FXMJOpts::initRoomOpts(Json::Value& jsOpts) {
	IMJOpts::initRoomOpts(jsOpts);

	m_bDianPaoOnePay = jsOpts["dpOnePay"].asBool();
	m_bEnable7Pair = jsOpts["pair7"].asBool();
	m_bEnableOnlyOneType = jsOpts["only1Type"].asBool();
	m_bEnableSB1 = jsOpts["sb1"].asBool();
	m_bEnableFollow = jsOpts["follow"].asBool();
	m_bEnableZha5 = jsOpts["zha5"].asBool();
	m_bEnableCool = jsOpts["cool"].asBool();
	m_bEnablePJH = jsOpts["pjh"].asBool();

	m_nFanLimit = 0;
	if (jsOpts["fanLimit"].isUInt()) {
		m_nFanLimit = jsOpts["fanLimit"].asUInt();
	}
	m_nGuang = 0;
	if (jsOpts["guang"].isUInt()) {
		m_nGuang = jsOpts["guang"].asUInt();
	}
}

uint16_t FXMJOpts::calculateDiamondNeed() {
#ifdef _DEBUG
	//return 0;
#endif // _DEBUG
	auto nLevel = getRoundLevel();
	if (nLevel > 3)
	{
		LOGFMTE("invalid room level for game = %u , level = %u", getGameType(), nLevel);
		nLevel = 1;
	}

	// is aa true ;
	if (isAA() || getPayType() == ePayType_AA) {
		uint16_t vAA[] = { 1 , 1 , 2 , 2 };
		return vAA[nLevel];
	}

	uint16_t vFangZhu[] = { 4 , 4, 6 , 6 };
	return vFangZhu[nLevel];
}

uint8_t FXMJOpts::calculateInitRound() {
	auto nLevel = getRoundLevel();
	if (nLevel > 3) {
		return 1;
	}

	if (nLevel > 7) {
		if (isCircle()) {
			return 1;
		}
		else {
			return 6;
		}
	}

	if (isCircle()) {
		if (nLevel > 3) {
			nLevel -= 4;
		}
	}
	else {
		if (nLevel < 4) {
			nLevel += 4;
		}
	}
	uint8_t vRounds[8] = { 1, 2, 3, 4, 6, 12, 18, 24 };
	return vRounds[nLevel];
}