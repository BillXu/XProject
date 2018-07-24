#include "IMJPlayer.h"
#include "MessageIdentifer.h"
#include "ServerDefine.h"
#include "log4z.h"
#include "AsyncRequestQuene.h"
#include "IMJPlayerCard.h"

void IMJPlayer::init(stEnterRoomData* pData, uint16_t nIdx )
{
	IGamePlayer::init(pData, nIdx);
	setState(eRoomPeer_WaitNextGame);
	m_nHuCnt = 0 ;
	m_nZiMoCnt = 0 ;
	m_nDianPaoCnt = 0 ;
	m_nMingGangCnt = 0 ;
	m_nAnGangCnt = 0 ;
	m_nHuaGangCnt = 0;
	getPlayerCard()->reset();
}

void IMJPlayer::onGameWillStart()
{
	IGamePlayer::onGameWillStart();
	m_nFlag = 0;
}

void IMJPlayer::onGameStart()
{
	IGamePlayer::onGameStart();
	setState(eRoomPeer_CanAct);
}

void IMJPlayer::onGameDidEnd()
{
	IGamePlayer::onGameDidEnd();
	setState(eRoomPeer_WaitNextGame);
	getPlayerCard()->reset();
}

void IMJPlayer::onGameEnd()
{
	IGamePlayer::onGameEnd();
	//setState(eRoomPeer_WaitNextGame);
	//clearGangFlag();
	//clearDecareBuGangFlag();
}

void IMJPlayer::signFlag(uint32_t nFlag)
{
	m_nFlag = m_nFlag | nFlag;
}

void IMJPlayer::clearFlag(uint32_t nFlag)
{
	m_nFlag = m_nFlag & (~nFlag);
}

bool IMJPlayer::haveFlag(uint32_t nFlag)
{
	return nFlag == (m_nFlag & nFlag);
}

void IMJPlayer::zeroFlag()
{
	m_nFlag = 0;
}

uint8_t IMJPlayer::getDianPaoCnt()
{
	return m_nDianPaoCnt;
}

void IMJPlayer::addDianPaoCnt()
{
	++m_nDianPaoCnt;
}

uint8_t IMJPlayer::getHuCnt()
{
	return m_nHuCnt;
}

void IMJPlayer::addHuCnt()
{
	++m_nHuCnt;
}

uint8_t IMJPlayer::getZiMoCnt()
{
	return m_nZiMoCnt;
}

void IMJPlayer::addZiMoCnt()
{
	++m_nZiMoCnt;
}

uint8_t IMJPlayer::getAnGangCnt()
{
	return m_nAnGangCnt;
}

void IMJPlayer::addAnGangCnt()
{
	++m_nAnGangCnt;
}

uint8_t IMJPlayer::getMingGangCnt()
{
	return m_nMingGangCnt;
}

void IMJPlayer::addMingGangCnt()
{
	++m_nMingGangCnt;
}

uint8_t IMJPlayer::getHuaGangCnt() {
	return m_nHuaGangCnt;
}

void IMJPlayer::addHuaGangCnt() {
	++m_nHuaGangCnt;
}