#pragma once
#include "IGameRecorder.h"
#include "CommonDefine.h"
class NiuNiuPlayerRecorder
	:public IPlayerRecorder
{
public:
	NiuNiuPlayerRecorder()
	{
		nBetTimes = 0;
		vHoldCards.clear();
	}

	bool toJson(Json::Value& js)override
	{
		IPlayerRecorder::toJson(js);
		js["base"] = nBetTimes;
		Json::Value jsCards;
		for (auto& ref : vHoldCards)
		{
			jsCards[jsCards.size()] = ref;
		}
		js["cards"] = jsCards;
		return true;
	}
	
	void setBetTimes(uint8_t nBetTimes)
	{
		this->nBetTimes = nBetTimes;
	}

	void setHoldCards( std::vector<uint8_t>& vHoldCards )
	{
		this->vHoldCards = vHoldCards;
	}
protected:
	uint8_t nBetTimes;  // zero means banker ;
	std::vector<uint8_t> vHoldCards;
};