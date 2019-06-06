#include "GoldenOpts.h"
#include "log4z.h"
#include "CommonDefine.h"
void GoldenOpts::initRoomOpts(Json::Value& jsOpts) {
	IPokerOpts::initRoomOpts(jsOpts);

	m_bEnable235 = jsOpts["enable235"].asBool();
	m_bEnableStraight = jsOpts["enableStraight"].asBool();
	m_bEnableXiQian = jsOpts["enableXiQian"].asBool();

	m_nBaseScore = 1;
	if (jsOpts["baseScore"].isUInt()) {
		m_nBaseScore = jsOpts["baseScore"].asUInt();
		if (m_nBaseScore == 0) {
			m_nBaseScore = 1;
		}
		if (m_nBaseScore > 10) {
			m_nBaseScore = 10;
		}
	}
	m_nMultiple = 10;
	if (jsOpts["multiple"].isUInt()) {
		m_nMultiple = jsOpts["multiple"].asUInt();
		if (m_nMultiple < 20) {
			m_nMultiple = 10;
		}else if (m_nMultiple > 100) {
			m_nMultiple = 10;
		}
		else {
			m_nMultiple = m_nMultiple / 10 * 10;
		}
	}
	m_nMustMenCircle = 0;
	if (jsOpts["mustMen"].isUInt()) {
		m_nMustMenCircle = jsOpts["mustMen"].asUInt();
	}
	m_nCanPKCircle = 0;
	if (jsOpts["pkCircleLimit"].isUInt()) {
		m_nCanPKCircle = jsOpts["pkCircleLimit"].asUInt();
	}
	m_nCircleLimit = 0;
	if (jsOpts["circleLimit"].isUInt()) {
		m_nCircleLimit = jsOpts["circleLimit"].asUInt();
	}
	m_nPKTimes = 1;
	if (jsOpts["pktimes"].isUInt()) {
		m_nPKTimes = jsOpts["pktimes"].asUInt();
		if (m_nPKTimes < 1 || m_nPKTimes > 5) {
			m_nPKTimes = 1;
		}
	}
	m_nWaitActTime = 0;
	if (jsOpts["waitActTime"].isUInt()) {
		m_nWaitActTime = jsOpts["waitActTime"].asUInt();
	}
}

uint16_t GoldenOpts::calculateDiamondNeed() {
#ifdef _DEBUG
	//return 0;
#endif // _DEBUG
	auto nLevel = getRoundLevel();
	if (nLevel >= 3)
	{
		LOGFMTE("invalid room level for game = %u , level = %u", getGameType(), nLevel);
		nLevel = 2;
	}

	// is aa true ;
	if (isAA() || getPayType() == ePayType_AA) {
		uint16_t vAA[] = { 1 , 2 , 3 };
		return vAA[nLevel] * 10 * 2;
	}

	if (getSeatCnt() == 9)
	{
		uint16_t vFangZhu[] = { 90 , 180 , 270 };
		return vFangZhu[nLevel];
	}

	if (12 == getSeatCnt())
	{
		uint16_t vFangZhu[] = { 120 , 240 , 360 };
		return vFangZhu[nLevel];
	}
	// 6,1 . 12.2 , 18. 3
	uint16_t vFangZhu[] = { 6 , 12 , 18 };
	return vFangZhu[nLevel] * 10;
}

uint8_t GoldenOpts::calculateInitRound() {
	uint8_t vJun[] = { 10 , 20 , 30 };
	auto nLevel = getRoundLevel();
	if (nLevel >= sizeof(vJun) / sizeof(uint8_t))
	{
		LOGFMTE("invalid level type = %u", nLevel);
		nLevel = 0;
	}
	return vJun[nLevel];
}