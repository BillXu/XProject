#pragma once
#include "IMJPlayer.h"
#include "TestMJPlayerCard.h"
class TestMJPlayer
	:public IMJPlayer
{
public:
	IMJPlayerCard* getPlayerCard()override;
protected:
	TestMJPlayerCard m_tPlayerCard;
};