#include "IMJOpts.h"
void IMJOpts::initRoomOpts(Json::Value& jsOpts) {
	IGameOpts::initRoomOpts(jsOpts);

	m_nBaseScore = 1;
	if (jsOpts["baseScore"].isUInt()) {
		m_nBaseScore = jsOpts["baseScore"].asUInt();
	}

	m_bCircle = jsOpts["circle"].asBool();
}