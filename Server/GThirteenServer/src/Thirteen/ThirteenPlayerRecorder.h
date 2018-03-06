#pragma once
#include "IGameRecorder.h"
#include "CommonDefine.h"
class ThirteenPlayerRecorder
	:public IPlayerRecorder
{
public:
	ThirteenPlayerRecorder()
	{
		vHoldCards.clear();
	}

	bool toJson(Json::Value& js)override
	{
		IPlayerRecorder::toJson(js);
		Json::Value jsCards, jsTypes;
		for (auto& ref : vHoldCards)
		{
			jsCards[jsCards.size()] = ref;
		}
		for (auto& ref : vTypes)
		{
			jsTypes[jsTypes.size()] = ref;
		}
		js["cards"] = jsCards;
		js["types"] = jsTypes;
		return true;
	}

	void setHoldCards( std::vector<uint8_t>& vHoldCards )
	{
		this->vHoldCards = vHoldCards;
	}

	void setTypes(std::vector<uint8_t>& vTypes)
	{
		this->vTypes = vTypes;
	}
protected:
	std::vector<uint8_t> vHoldCards;
	std::vector<uint8_t> vTypes;
};