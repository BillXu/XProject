#include "GDOpts.h"
#include "log4z.h"
#include "CommonDefine.h"
void GDOpts::initRoomOpts(Json::Value& jsOpts) {
	IPokerOpts::initRoomOpts(jsOpts);

	m_bEnableDouble = jsOpts["double"].asBool();
	m_bRandomSeat = jsOpts["randomSeat"].asBool();
	m_bRandomDa = jsOpts["randomDa"].asBool();
	if (m_bRandomSeat && m_bRandomDa == false) {
		LOGFMTE("when random seat must random da game = %u", getGameType());
		m_bRandomDa = true;
	}

	m_nBaseScore = 1;
	if (jsOpts["baseScore"].isUInt()) {
		m_nBaseScore = jsOpts["baseScore"].asUInt();
		if (m_nBaseScore == 0 || m_nBaseScore > 100) {
			m_nBaseScore = 1;
		}
	}

	if (getSeatCnt() != 4) {
		LOGFMTE("invalid seat cnt game = %u , seatCnt = %u", getGameType(), getSeatCnt());
		setSeatCnt(4);
	}
}

uint16_t GDOpts::calculateDiamondNeed() {
	//#ifdef _DEBUG
	//	return 0;
	//#endif // _DEBUG
	return 0;

	auto nLevel = getRoundLevel();
	if (nLevel > 2)
	{
		LOGFMTE("invalid room level for game = %u , level = %u", getGameType(), nLevel);
		nLevel = 0;
	}

	// is aa true ;
	if (isAA() || getPayType() == ePayType_AA) {
		uint16_t vAA[] = { 1 , 2 , 3 };
		return vAA[nLevel];
	}

	// 6,1 . 12.2 , 18. 3
	uint16_t vFangZhu[] = { 4 , 8 , 12 };
	return vFangZhu[nLevel];
}

uint8_t GDOpts::calculateInitRound() {
	uint8_t vJun[] = { 3 , 5 , 7 };
	auto nLevel = getRoundLevel();
	if (nLevel >= sizeof(vJun) / sizeof(uint8_t))
	{
		LOGFMTE("invalid level type = %u", nLevel);
		nLevel = 0;
	}
	return vJun[nLevel];
}