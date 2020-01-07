#pragma once
#include "CommonDefine.h"
#include "json\json.h"
#include "log4z.h"
class IOpts
{
public:
	bool init(Json::Value opts)
	{
		jsOpts = opts;
	}

	ePayRoomCardType payType()
	{
		if ( jsOpts["payType"].isNull() )
		{
			LOGFMTW( "payType key is null" );
			return ePayRoomCardType::ePayType_AA;
		}
		return (ePayRoomCardType)jsOpts["payType"].asUInt();
	}

	uint8_t seatCnt()
	{
		if (jsOpts["seatCnt"].isNull())
		{
			LOGFMTW("seatCnt key is null");
			return 4;
		}
		return jsOpts["seatCnt"].asUInt();
	}

	bool isCircle()
	{
		if (jsOpts["circle"].isNull())
		{
			LOGFMTW("circle key is null");
			return false;
		}
		return jsOpts["circle"].asUInt() == 1;
	}
 
	uint8_t roundCnt()
	{
		if ( jsOpts["level"].isNull() )
		{
			jsOpts["level"] = 0;
			LOGFMTE("round key is null");
		}
		return jsOpts["level"].asUInt() == 0 ? 8 : 16;
	}

	eGameType gameType()
	{
		if (jsOpts["gameType"].isNull())
		{
			LOGFMTW("gameType key is null");
			return eGameType::eGame_Max;
		}
		return (eGameType)jsOpts["gameType"].asUInt();
	}

	uint8_t baseScore()
	{
		if (jsOpts["baseScore"].isNull())
		{
			LOGFMTW("baseScore key is null");
			return 1;
		}
		return jsOpts["baseScore"].asUInt();
	}
 
	bool isAvoidCheat()
	{
		if (jsOpts["enableAvoidCheat"].isNull())
		{
			LOGFMTW("enableAvoidCheat key is null");
			return false;
		}

		return jsOpts["enableAvoidCheat"] == 1;
	}
 
	bool isForceGPS()
	{
		if (jsOpts["gps"].isNull())
		{
			LOGFMTW("gps key is null");
			return false;
		}
		return jsOpts["gps"] == 1;
	}

	uint8_t	getDiamondFee()
	{
		return 0;
	}

protected:
	Json::Value jsOpts;
};