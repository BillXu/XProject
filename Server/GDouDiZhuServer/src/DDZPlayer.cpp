#include "DDZPlayer.h"
void DDZPlayer::onGameWillStart()
{
	IGamePlayer::onGameWillStart();
	getPlayerCard()->reset();
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
