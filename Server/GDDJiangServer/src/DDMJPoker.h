#pragma once
#include "IMJPoker.h"
class DDMJPoker
	:public IMJPoker
{
public:
	void init(Json::Value& jsOpt)override;
	uint8_t getCardByIdx(uint16_t nIdx, bool isReverse = false);

protected:
	void makeSpecialCard(std::vector<uint8_t>& vMakedCards)override;
};