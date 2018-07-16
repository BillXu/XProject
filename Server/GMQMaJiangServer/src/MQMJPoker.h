#pragma once
#include "IMJPoker.h"
class MQMJPoker
	:public IMJPoker
{
public:
	void init(Json::Value& jsOpt)override;
};