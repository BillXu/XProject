#pragma once
#include "NativeTypes.h"
struct stEnterRoomData
{
	uint32_t nUserUID;
	uint32_t nSessionID;
	int32_t nChip;  // most situation equal coin ;
	uint32_t nDiamond;
	uint32_t nCurInRoomID;
	uint32_t nClubID = 0;
	uint32_t nRoomIdx = -1;
};
