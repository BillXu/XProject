#include "DDZPlayer.h"
void DDZPlayer::onGameWillStart()
{
	IGamePlayer::onGameWillStart();
	getPlayerCard()->reset();
}

DDZPlayerCard* DDZPlayer::getPlayerCard()
{
	return &m_tPeerCard;
}

bool DDZPlayer::isMingPai()
{
	return haveState(eRoomPeer_ShowedHoldCard);
}

void DDZPlayer::doMingPai()
{
	addState(eRoomPeer_ShowedHoldCard);
}

bool DDZPlayer::isTuoGuan()
{
	return haveState(eRoomPeer_SysAutoAct);
}

void DDZPlayer::setTuoGuanFlag(uint8_t isTuoGuan)
{
	if (isTuoGuan)
	{
		addState(eRoomPeer_SysAutoAct);
	}
	else
	{
		clearState(eRoomPeer_SysAutoAct);
	}
}
