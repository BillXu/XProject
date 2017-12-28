#pragma once
#include "IGameRecorder.h"
#include "CommonDefine.h"
class GoldenPlayerRecorder
	:public IPlayerRecorder
{
public:
	GoldenPlayerRecorder()
	{
		vHoldCards.clear();
	}

	bool toJson(Json::Value& js)override
	{
		IPlayerRecorder::toJson(js);
		Json::Value jsCards;
		for (auto& ref : vHoldCards)
		{
			jsCards[jsCards.size()] = ref;
		}
		js["cards"] = jsCards;
		return true;
	}

	void setHoldCards( std::vector<uint8_t>& vHoldCards )
	{
		this->vHoldCards = vHoldCards;
	}
protected:
	std::vector<uint8_t> vHoldCards;
};