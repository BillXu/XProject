#include "CoinRedBlack\RedBlackPlayer.h"
bool RedBlackPlayer::doBet(int32_t nBetCoin, eBetPool ePool)
{
	if ( getBetCoin() + nBetCoin > getChips() )
	{
		return false;
	}

	if ( ePool >= eBet_Max )
	{
		return false;
	}
	m_vBetedCoin[ePool] += nBetCoin;
	return true;
}

void RedBlackPlayer::onGameEnd()
{
	stBetRecorder st;
	st.nBetCoin = getBetCoin();
	st.nOffset = getSingleOffset();
	m_vRecorders.push_back(st);
	if ( m_vRecorders.size() > 20 )
	{
		m_vRecorders.pop_front();
	}
	
	memset(m_vBetedCoin, 0, sizeof(m_vBetedCoin));
	IGamePlayer::onGameEnd();
}

void RedBlackPlayer::onGameDidEnd()
{
	memset(m_vBetedCoin,0 , sizeof(m_vBetedCoin) );
	IGamePlayer::onGameDidEnd();
}

int32_t RedBlackPlayer::getBetCoin()
{
	int32_t nBetCoin = 0;
	for (auto& ref : m_vBetedCoin)
	{
		nBetCoin += ref;
	}
	return nBetCoin;
}

int8_t RedBlackPlayer::getWinTimes()
{
	uint8_t nTimes = 0;
	for (auto& ref : m_vRecorders)
	{
		if (ref.nOffset > 0)
		{
			++nTimes;
		}
	}
	return nTimes;
}
