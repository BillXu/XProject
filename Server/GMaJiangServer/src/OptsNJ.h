#pragma once
#include "../GameCommon/src/IOpts.h"
class OptsNJ 
	: public IOpts
{
public:
	bool optsJieZhuangBi()
	{
		if ( this->jsOpts["jieZhuangBi"].isNull() == false)
		{
			return jsOpts["jieZhuangBi"].asUInt() == 1;
		}
		return false;
	}

	bool optsHuaZa2()
	{
		if (this->jsOpts["huaZa2"].isNull() == false)
		{
			return jsOpts["huaZa2"].asUInt() == 1;
		}
		return false;
	}

	bool optsSiLianFeng()
	{
		if (this->jsOpts["siLianFeng"].isNull() == false)
		{
			return jsOpts["siLianFeng"].asUInt() == 1;
		}
		return false;
	}

	bool optsBaoMi()
	{
		if ( this->jsOpts["baoMi"].isNull() == false )
		{
			return jsOpts["baoMi"].asUInt() == 1;
		}
		return false;
	}

	bool optsTuoGuan()
	{
		if (this->jsOpts["tuoGuan"].isNull() == false)
		{
			return jsOpts["tuoGuan"].asUInt() == 1;
		}
		return false;
	}

	bool optsJinYuanZi()
	{
		if (this->jsOpts["jinYuanZi"].isNull() == false)
		{
			return jsOpts["jinYuanZi"].asUInt() == 1;
		}
		return false;
	}

	uint8_t enterScore()
	{
		if (this->jsOpts["enterScore"].isNull() == false)
		{
			return jsOpts["enterScore"].asUInt();
		}
		return 100;
	}
};