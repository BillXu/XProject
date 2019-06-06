#include "TestMJOpts.h"
void TestMJOpts::initRoomOpts(Json::Value& jsOpts) {
	IMJOpts::initRoomOpts(jsOpts);
}

uint16_t TestMJOpts::calculateDiamondNeed() {
	return 0;
}

uint8_t TestMJOpts::calculateInitRound() {
	return 3;
}