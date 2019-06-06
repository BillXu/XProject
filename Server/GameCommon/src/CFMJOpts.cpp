#include "CFMJOpts.h"
#include "log4z.h"
#include "CommonDefine.h"
void CFMJOpts::initRoomOpts(Json::Value& jsOpts) {
	IMJOpts::initRoomOpts(jsOpts);

	m_nFanLimit = 0;
	if (jsOpts["fanLimit"].isUInt()) {
		m_nFanLimit = jsOpts["fanLimit"].asUInt();
	}
}

uint16_t CFMJOpts::calculateDiamondNeed() {
#ifdef _DEBUG
	return 0;
#endif // _DEBUG

	auto nLevel = getRoundLevel();

	if (nLevel > 5)
	{
		LOGFMTE("invalid room level for game = %u , level = %u", getGameType(), nLevel);
		nLevel = 1;
	}

	// is aa true ;
	if (isAA() || ePayType_AA == getPayType())
	{
		uint16_t vAA[] = { 1 , 2 , 1 , 1 , 2 , 2 };
		return vAA[nLevel];
	}

	uint16_t vFangZhu[] = { 4 , 8 , 2 , 4 , 6 , 8 };
	return vFangZhu[nLevel];
}

uint8_t CFMJOpts::calculateInitRound() {
#ifdef _DEBUG
	return 2;
#endif // _DEBUG

	auto nLevel = getRoundLevel();
	if (isCircle()) {
		if (nLevel < 2 || nLevel > 5) {
			nLevel = 2;
		}
	}
	else {
		if (nLevel > 1) {
			nLevel = 0;
		}
	}

	uint8_t vRounds[6] = { 8, 16, 1, 2, 3, 4 };
	return vRounds[nLevel];
}