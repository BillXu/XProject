#pragma once
#include "IGamePlayer.h"
#include "DDZPlayerCard.h"
class DDZPlayer
	:public IGamePlayer
{
public:
	DDZPlayer() { m_isChaoZhuang = false; }
	void onGameWillStart()override;
	void onGameDidEnd()override;
	DDZPlayerCard* getPlayerCard();
	bool isMingPai();
	void doMingPai();
	bool isTuoGuan();
	void setTuoGuanFlag( uint8_t isTuoGuan );
	void doChaoZhuang();
	bool isChaoZhuang();
	void doTiLaChuai();
	bool isTiLaChuai();
protected:
	DDZPlayerCard m_tPeerCard;
	bool m_isChaoZhuang;
};