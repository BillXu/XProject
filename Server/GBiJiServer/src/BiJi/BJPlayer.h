#pragma once
#include "IGamePlayer.h"
#include "BiJi\BJPlayerCard.h"
class BJPlayer
	:public IGamePlayer
{
public:
	void onGameWillStart()override;
	BJPlayerCard* getPlayerCard();
	void addOffsetPerGuo( uint8_t nGuoIdx , int32_t nOffset );
	int32_t getOffsetPerGuo( uint8_t nGuoIdx );
	void addXiPaiOffset( int32_t nOffset );
	void addTongGuanOffset( int32_t nOffset );
	int32_t getXiPaiOffset();
	int32_t getTongGuanOffset();
	bool isTongGuan();
protected:
	BJPlayerCard m_tPeerCard;
	int32_t m_vGuoOffset[3];
	int32_t m_nXiPaiOffset;
	int32_t nTongGuanOffset;
};