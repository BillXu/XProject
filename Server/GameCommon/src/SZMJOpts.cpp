#include "SZMJOpts.h"
#include "log4z.h"
#include "CommonDefine.h"
void SZMJOpts::initRoomOpts(Json::Value& jsOpts) {
	IMJOpts::initRoomOpts(jsOpts);

	m_bEnableHaoHuaQiDui = jsOpts["haoQiDui"].asBool();
	m_bEnableDiLing = jsOpts["diLing"].asBool();

	m_nRuleMode = 1;
	if (jsOpts["ruleMode"].isUInt()) {
		m_nRuleMode = jsOpts["ruleMode"].asUInt();
		if (m_nRuleMode != 1 && m_nRuleMode != 2) {
			LOGFMTE("invalid rule mode value = %u, game type = %n", m_nRuleMode, getGameType());
			m_nRuleMode = 1;
		}
	}
}

uint16_t SZMJOpts::calculateDiamondNeed() {
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

uint8_t SZMJOpts::calculateInitRound() {
	auto nLevel = getRoundLevel();
	if (nLevel > 2) {
		LOGFMTE("invalid room level for game = %u , level = %u", getGameType(), nLevel);
		nLevel = 0;
	}

	uint8_t vRounds[3] = { 4 , 8 , 16 };
	return vRounds[nLevel];
}