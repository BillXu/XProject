#pragma once
#include "IGamePlayer.h"
#include "DDZPlayerCard.h"
class DDZPlayer
	:public IGamePlayer
{
public:
	void onGameWillStart()override;
	DDZPlayerCard* getPlayerCard();
	bool isMingPai();
	void doMingPai();
	bool isTuoGuan();
	void setTuoGuanFlag( uint8_t isTuoGuan );
protected:
	DDZPlayerCard m_tPeerCard;
};