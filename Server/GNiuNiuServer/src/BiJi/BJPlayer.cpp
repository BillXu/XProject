#include "BJPlayer.h"
#include "log4z.h"
void BJPlayer::onGameWillStart()
{
	IGamePlayer::onGameWillStart();
	getPlayerCard()->reset();
	memset(m_vGuoOffset,0,sizeof(m_vGuoOffset));
	m_nXiPaiOffset = nTongGuanOffset = 0;
}

BJPlayerCard* BJPlayer::getPlayerCard()
{
	return &m_tPeerCard;
}

void BJPlayer::addOffsetPerGuo(uint8_t nGuoIdx, int32_t nOffset)
{
	if (nGuoIdx >= 3)
	{
		LOGFMTE( "invalid guo idx = %u",nGuoIdx );
		return ;
	}

	addSingleOffset(nOffset);
	m_vGuoOffset[nGuoIdx] = nOffset;
}

int32_t BJPlayer::getOffsetPerGuo(uint8_t nGuoIdx)
{
	if (nGuoIdx >= 3)
	{
		LOGFMTE("invalid guo idx = %u", nGuoIdx);
		return 0;
	}
	return m_vGuoOffset[nGuoIdx];
}

void BJPlayer::addXiPaiOffset( int32_t nOffset )
{
	m_nXiPaiOffset += nOffset;
	addSingleOffset(nOffset);
}

void BJPlayer::addTongGuanOffset( int32_t nOffset )
{
	nTongGuanOffset += nOffset;
	addSingleOffset(nOffset);
}

int32_t BJPlayer::getXiPaiOffset()
{
	return m_nXiPaiOffset;
}

int32_t BJPlayer::getTongGuanOffset()
{
	return nTongGuanOffset;
}

bool BJPlayer::isTongGuan()
{
	for (auto& ref : m_vGuoOffset)
	{
		if (ref <= 0)
		{
			return false;
		}
	}
	return true;
}