#include "GDPlayer.h"
void GDPlayer::init(IGameRoom* pRoom, stEnterRoomData* pData, uint16_t nIdx) {
	IGamePlayer::init(pRoom, pData, nIdx);
	m_nExtraTime = 0;
}

void GDPlayer::onGameWillStart()
{
	IGamePlayer::onGameWillStart();
	getPlayerCard()->reset();
	clearDouble();
}

void GDPlayer::onGameDidEnd()
{
	IGamePlayer::onGameDidEnd();
	m_isChaoZhuang = false;
}

GDPlayerCard* GDPlayer::getPlayerCard()
{
	return &m_tPeerCard;
}

bool GDPlayer::isMingPai()
{
	return haveState(eRoomPeer_ShowedHoldCard);
}

void GDPlayer::doMingPai()
{
	addState(eRoomPeer_ShowedHoldCard);
}

bool GDPlayer::isTuoGuan()
{
	return haveState(eRoomPeer_SysAutoAct);
}

void GDPlayer::setTuoGuanFlag(uint8_t isTuoGuan)
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

void GDPlayer::doChaoZhuang()
{
	m_isChaoZhuang = true;
}

bool GDPlayer::isChaoZhuang()
{
	return m_isChaoZhuang;
}

void GDPlayer::doTiLaChuai()  
{
	addState(eRoomPeer_TiLaChuai);
}

bool GDPlayer::isTiLaChuai()
{
	return haveState(eRoomPeer_TiLaChuai);
}

void GDPlayer::addExtraTime(float fTime) {
	m_nExtraTime += fTime;
}