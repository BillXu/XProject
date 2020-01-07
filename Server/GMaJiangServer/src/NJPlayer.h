#pragma once
#include "IMJPlayer.h"
#include "NJPlayerCard.h"
class NJPlayer
	:public IMJPlayer
{
public:
	IMJPlayerCard* getPlayerCard()override;
protected:
	NJPlayerCard m_tPlayerCard;
};