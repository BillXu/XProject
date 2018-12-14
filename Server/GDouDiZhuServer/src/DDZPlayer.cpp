#include "DDZPlayer.h"
void DDZPlayer::onGameWillStart()
{
	IGamePlayer::onGameWillStart();
	getPlayerCard()->reset();
	clearDouble();
}

void DDZPlayer::onGameDidEnd()
{
	IGamePlayer::onGameDidEnd();
	m_isChaoZhuang = false;
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

void DDZPlayer::doChaoZhuang()
{
	m_isChaoZhuang = true;
}

bool DDZPlayer::isChaoZhuang()
{
	return m_isChaoZhuang;
}

void DDZPlayer::doTiLaChuai()  
{
	addState(eRoomPeer_TiLaChuai);
}

bool DDZPlayer::isTiLaChuai()
{
	return haveState(eRoomPeer_TiLaChuai);
}
