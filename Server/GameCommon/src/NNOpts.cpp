#include "NNOpts.h"
#include "log4z.h"
#include "CommonDefine.h"
void NNOpts::initRoomOpts(Json::Value& jsOpts) {
	IPokerOpts::initRoomOpts(jsOpts);

	m_bEnableWuHua = jsOpts["wuHua"].asBool();
	m_bEnableZhaDan = jsOpts["zhaDan"].asBool();
	m_bEnableWuXiao = jsOpts["wuXiao"].asBool();
	m_bEnableShunKan = jsOpts["shunKan"].asBool();
	m_bEnableFengKuang = jsOpts["fengKuang"].asBool();
	m_bEnableTuiZhu = jsOpts["tuiZhu"].asBool();
	m_bEnableTuiZhuang = jsOpts["tuiZhuang"].asBool();

	m_nBaseScore = 1;
	if (jsOpts["diFen"].isUInt()) {
		m_nBaseScore = jsOpts["diFen"].asUInt();
	}
	m_nBankType = 0;
	if (jsOpts["bankType"].isUInt()) {
		m_nBankType = jsOpts["bankType"].asUInt();
	}
	m_nFanBei = 0;
	if (jsOpts["fanBei"].isUInt()) {
		m_nFanBei = jsOpts["fanBei"].asUInt();
	}
}

uint16_t NNOpts::calculateDiamondNeed() {
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
		return vAA[nLevel] * 10;
	}

	if (getSeatCnt() == 8)
	{
		// 6,1 . 12.2 , 18. 3
		uint16_t vFangZhu[] = { 4 , 8 , 12 };
		return vFangZhu[nLevel] * 10;
	}
	// 6,1 . 12.2 , 18. 3
	uint16_t vFangZhu[] = { 3 , 6 , 9 };
	return vFangZhu[nLevel] * 10;
}

uint8_t NNOpts::calculateInitRound() {
	uint8_t vJun[] = { 10 , 20 , 30 };
	auto nLevel = getRoundLevel();
	if (nLevel >= sizeof(vJun) / sizeof(uint8_t))
	{
		LOGFMTE("invalid level type = %u", nLevel);
		nLevel = 0;
	}
	return vJun[nLevel];
}