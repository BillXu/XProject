#pragma once
#include "NativeTypes.h"
#include "CommonDefine.h"
#include "IMJPlayerCard.h"
class IMJRoom;
class IMJPlayerCard;
class IMJPlayer;
class IFanxing
{
public:
	virtual uint16_t getFanxingType() = 0;
	virtual bool checkFanxing( IMJPlayerCard* pPlayerCard, IMJPlayer* pPlayer , uint8_t nInvokerIdx,IMJRoom* pmjRoom ) = 0 ;
};
