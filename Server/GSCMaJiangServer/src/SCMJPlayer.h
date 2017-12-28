#pragma once
#include "IMJPlayer.h"
#include "SCMJPlayerCard.h"
class SCMJPlayer
	:public IMJPlayer
{
public:
	IMJPlayerCard* getPlayerCard()override;
	void onGameWillStart()override;
	void clearGangFlag() { clearFlag(eMJActFlag_Gang); }
	bool haveGangFlag() { return haveFlag(eMJActFlag_Gang); }
protected:
	SCMJPlayerCard m_tPlayerCard;
};