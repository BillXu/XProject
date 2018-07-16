#include "LuoMJPoker.h"
void LuoMJPoker::init(Json::Value& jsOpt) {
	IMJPoker::init(jsOpt);

	// every card are 4 count 
	for (uint8_t nCnt = 0; nCnt < 4; ++nCnt)
	{
		addCardToPoker(makeCardNumber(eCT_Jian, 1));
	}
}