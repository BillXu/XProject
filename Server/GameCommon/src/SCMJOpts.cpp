#include "SCMJOpts.h"
void SCMJOpts::initRoomOpts(Json::Value& jsOpts) {
	IMJOpts::initRoomOpts(jsOpts);

	m_bEnableBloodFlow = jsOpts["enableBloodFlow"].asBool();
	m_bZiMoAddFan = jsOpts["enableZMAF"].asBool();
	m_bDianGangZiMo = jsOpts["enableDGZM"].asBool();
	m_bEnableExchange3Cards = jsOpts["enable3Cards"].asBool();
	m_bEnable19 = jsOpts["enable19"].asBool();
	m_bEnableMenQing = jsOpts["enableMQZZ"].asBool();
	m_bEnableZZ = jsOpts["enableMQZZ"].asBool();
	m_bEnableTDHu = jsOpts["enableTDHu"].asBool();

	m_nFanLimit = 0;
	if (jsOpts["fanLimit"].isUInt()) {
		m_nFanLimit = jsOpts["fanLimit"].asUInt();
	}
}

uint16_t SCMJOpts::calculateDiamondNeed() {
	return 0;
}

uint8_t SCMJOpts::calculateInitRound() {
	return 3;
}