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
protected:
	DDZPlayerCard m_tPeerCard;
	bool m_isMingPai;
};