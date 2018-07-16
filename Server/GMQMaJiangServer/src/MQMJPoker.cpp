#include "MQMJPoker.h"
void MQMJPoker::init(Json::Value& jsOpt) {
	IMJPoker::init(jsOpt);

	// every card are 4 count 
	for (uint8_t nCnt = 0; nCnt < 4; ++nCnt)
	{
		// add feng
		for (uint8_t nValue = 1; nValue <= 4; ++nValue) {
			addCardToPoker(makeCardNumber(eCT_Feng, nValue));
		}

		// add jian
		for (uint8_t nValue = 1; nValue <= 3; ++nValue) {
			addCardToPoker(makeCardNumber(eCT_Jian, nValue));
		}
	}
}