#pragma once
#include "IMJPoker.h"
class LuoMJPoker
	:public IMJPoker
{
public:
	void init(Json::Value& jsOpt)override;
};