#include "BJOpts.h"
#include "log4z.h"
#include "CommonDefine.h"
void BJOpts::initRoomOpts(Json::Value& jsOpts) {
	IPokerOpts::initRoomOpts(jsOpts);

	m_bEnableSanQing = jsOpts["isSQ"].asBool();
	m_bEnableShunQingDaTou = jsOpts["isShunqing"].asBool();
	m_bTianJiSaiMa = jsOpts["playWay"].asBool();
	m_bEnableGiveUp = jsOpts["isGiveUp"].asBool();

	m_nRate = 0;
	if (jsOpts["times"].isUInt()) {
		m_nRate = jsOpts["times"].asUInt();
	}
}

uint16_t BJOpts::calculateDiamondNeed() {
#ifdef _DEBUG
	return 0;
#endif // _DEBUG

	auto nLevel = getRoundLevel();
	if (nLevel >= 3)
	{
		LOGFMTE("invalid room level for game = %u , level = %u", getGameType(), nLevel);
		nLevel = 2;
	}

	// is aa true ;
	if (isAA() || getPayType() == ePayType_AA) {
		uint8_t vAA[] = { 10 , 20 , 30 };
		return vAA[nLevel];
	}

	if (ePayType_Winer == getPayType())
	{
		uint8_t vAA[] = { 5 , 10 , 15 };
		return vAA[nLevel] * 10;
	}

	uint8_t vFangZhu[] = { 4 , 8 , 12 };
	return vFangZhu[nLevel] * 10;
}

uint8_t BJOpts::calculateInitRound() {
	auto nLevel = getRoundLevel();
	uint8_t vJun[] = { 10 , 20 , 30 };
	if (nLevel >= sizeof(vJun) / sizeof(uint8_t))
	{
		LOGFMTE("invalid level type = %u", nLevel);
		nLevel = 0;
	}
	return vJun[nLevel];
}