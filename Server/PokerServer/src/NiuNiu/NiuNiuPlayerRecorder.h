#pragma once
#include "IGameRecorder.h"
#include "CommonDefine.h"
class NiuNiuPlayerRecorder
	:public IPlayerRecorder
{
public:
	NiuNiuPlayerRecorder(uint32_t nUserUID, int32_t nOffset) : IPlayerRecorder(nUserUID,nOffset){}
	bool toJson(Json::Value& js)override
	{
		IPlayerRecorder::toJson(js);
		js["base"] = nBetTimes;
		Json::Value jsCards;
		for (auto& ref : vCards)
		{
			jsCards[jsCards.size()] = ref;
		}
		js["cards"] = jsCards;
		return true;
	}
public:
	uint8_t nBetTimes;  // zero means banker ;
	uint32_t vCards[NIUNIU_HOLD_CARD_COUNT];
};