#include "LuoMJOpts.h"
#include "log4z.h"
#include "CommonDefine.h"
void LuoMJOpts::initRoomOpts(Json::Value& jsOpts) {
	IMJOpts::initRoomOpts(jsOpts);

	m_bDianPaoOnePay = jsOpts["dpOnePay"].asBool();
	m_bEnableCaiGang = jsOpts["caiGang"].asBool();
	m_bEnableHunPiao = jsOpts["hunPiao"].asBool();
	m_bEnableSB1 = jsOpts["sb1"].asBool();

	m_nFanLimit = 0;
	if (jsOpts["fanLimit"].isUInt()) {
		m_nFanLimit = jsOpts["fanLimit"].asUInt();
	}
	m_nGuang = 0;
	if (jsOpts["guang"].isUInt()) {
		m_nGuang = jsOpts["guang"].asUInt();
	}
}

uint16_t LuoMJOpts::calculateDiamondNeed() {
#ifdef _DEBUG
	//return 0;
#endif // _DEBUG
	auto nLevel = getRoundLevel();
	if (nLevel > 1)
	{
		LOGFMTE("invalid room level for game = %u , level = %u", getGameType(), nLevel);
		nLevel = 1;
	}

	// is aa true ;
	if (isAA() || getPayType() == ePayType_AA) {
		uint16_t vAA[] = { 1 , 2 };
		return vAA[nLevel];
	}

	uint16_t vFangZhu[] = { 4 , 8 };
	return vFangZhu[nLevel];
}

uint8_t LuoMJOpts::calculateInitRound() {
	auto nLevel = getRoundLevel();
	if (nLevel > 1) {
		return 1;
	}
	uint8_t vRounds[2] = { 8, 16 };
	return vRounds[nLevel];
}