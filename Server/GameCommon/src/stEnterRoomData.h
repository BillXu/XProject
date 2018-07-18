#pragma once
#include "NativeTypes.h"
struct stEnterRoomData
{
	uint32_t nUserUID;
	uint32_t nSessionID;
	uint32_t nChip;  // most situation equal coin ;
	uint32_t nDiamond;
	uint32_t nCurInRoomID;
	bool isRobot; // isRobot ;
	stEnterRoomData() { isRobot = false; }
};
