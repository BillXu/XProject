#include "BJPlayer.h"
#include "log4z.h"
#include "BJPlayerRecorder.h"
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
	m_vGuoOffset[nGuoIdx] += nOffset;

	auto& ref = nOffset > 0 ? m_nPartWin : m_nPartLose;
	ref += nOffset;
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

	m_nPartXiQian += nOffset;
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

int32_t BJPlayer::getPartWin()
{
	return m_nPartWin;
}

int32_t BJPlayer::getPartLose()
{
	return m_nPartLose;
}

int32_t BJPlayer::getPartXiPai()
{
	return m_nPartXiQian;
}

bool BJPlayer::recorderVisitor(std::shared_ptr<IPlayerRecorder> ptrPlayerReocrder)
{
	IGamePlayer::recorderVisitor(ptrPlayerReocrder);
	auto pRecorder = (BJPlayerRecorder*)ptrPlayerReocrder.get();

	std::vector<uint8_t> vHold,vTemp;
	for (uint8_t nIdx = 0; nIdx < 3; ++nIdx)
	{
		uint8_t nType = 0;
		getPlayerCard()->getGroupInfo(nIdx,nType,vTemp);
		vHold.insert(vHold.end(),vTemp.begin(),vTemp.end() );
		vTemp.clear();
	}
	pRecorder->setHoldCards(vHold);
	return true;
}