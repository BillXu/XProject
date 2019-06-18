#include "YZMJOpts.h"
#include "log4z.h"
#include "CommonDefine.h"
void YZMJOpts::initRoomOpts(Json::Value& jsOpts) {
	IMJOpts::initRoomOpts(jsOpts);

	m_bPeiZi = jsOpts["isPeiZi"].asBool();
	m_bBaiBanPeiZi = jsOpts["isBaiBanPeiZi"].asBool();
	m_bYiTiaoLong = jsOpts["haveYiTiaoLong"].asBool();
	m_bQiDui = jsOpts["have7Pair"].asBool();

	m_nGuang = 0;
	if (jsOpts["guang"].isUInt()) {
		m_nGuang = jsOpts["guang"].asUInt();
	}
}

uint16_t YZMJOpts::calculateDiamondNeed() {
#ifdef _DEBUG
	return 0;
#endif // _DEBUG
	auto nLevel = getRoundLevel();
	if (nLevel > 2) {
		LOGFMTE("invalid room level for game = %u , level = %u", getGameType(), nLevel);
		nLevel = 0;
	}

	// is aa true ;
	if (isAA() || getPayType() == ePayType_AA) {
		uint16_t vAA[] = { 1 , 1 , 2 };
		return vAA[nLevel];
	}

	uint16_t vFangZhu[] = { 2 , 4 , 8 };
	return vFangZhu[nLevel];
}

uint8_t YZMJOpts::calculateInitRound() {
	auto nLevel = getRoundLevel();
	if (nLevel > 2) {
		LOGFMTE("invalid room level for game = %u , level = %u", getGameType(), nLevel);
		nLevel = 0;
	}

	uint8_t vRounds[3] = { 4 , 8 , 16 };
	if (isCircle()) {
		vRounds[0] = 1;
		vRounds[1] = 2;
		vRounds[2] = 4;
	}
	return vRounds[nLevel];
}