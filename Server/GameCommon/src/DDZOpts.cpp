#include "DDZOpts.h"
#include "log4z.h"
#include "CommonDefine.h"
void DDZOpts::initRoomOpts(Json::Value& jsOpts) {
	IPokerOpts::initRoomOpts(jsOpts);

	m_bEnableDouble = jsOpts["double"].asBool();
	m_bNotShuffle = jsOpts["notShuffle"].asBool();
	m_bEnableChaoZhuang = jsOpts["isChaoZhuang"].asBool();

	m_nFengDing = 0;
	if (jsOpts["maxBet"].isUInt()) {
		m_nFengDing = jsOpts["maxBet"].asUInt();
	}
	m_nRotLandlordType = 0;
	if (jsOpts["rlt"].isUInt()) {
		m_nRotLandlordType = jsOpts["rlt"].asUInt();
	}
	m_nBaseScore = 1;
	if (jsOpts["baseScore"].isUInt()) {
		m_nBaseScore = jsOpts["baseScore"].asUInt();
		if (m_nBaseScore == 0 || m_nBaseScore > 100) {
			m_nBaseScore = 1;
		}
	}
	m_nFanLimit = 0;
	if (jsOpts["fanLimit"].isUInt()) {
		m_nFanLimit = jsOpts["fanLimit"].asUInt();
	}
}

uint16_t DDZOpts::calculateDiamondNeed() {
//#ifdef _DEBUG
//	return 0;
//#endif // _DEBUG
	auto nLevel = getRoundLevel();
	if (nLevel >= 5)
	{
		LOGFMTE("invalid room level for game = %u , level = %u", getGameType(), nLevel);
		nLevel = 0;
	}

	// is aa true ;
	if (isAA() || getPayType() == ePayType_AA) {
		uint16_t vAA[] = { 1 , 2 , 3 , 1 , 2 };
		if (nLevel < 3) {
			return vAA[nLevel] * 10;
		}
		return vAA[nLevel];
	}

	// 6,1 . 12.2 , 18. 3
	uint16_t vFangZhu[] = { 3 , 6 , 9 , 3 , 6 };
	if (nLevel < 3) {
		return vFangZhu[nLevel] * 10;
	}
	return vFangZhu[nLevel];
}

uint8_t DDZOpts::calculateInitRound() {
	uint8_t vJun[] = { 9 , 18 , 27 , 8 , 16 };
	auto nLevel = getRoundLevel();
	if (nLevel >= sizeof(vJun) / sizeof(uint8_t))
	{
		LOGFMTE("invalid level type = %u", nLevel);
		nLevel = 0;
	}
	return vJun[nLevel];
}