#pragma once
#include "NativeTypes.h"
#include "json\json.h"
class IPoker
{
public:
	IPoker() { m_nCurIdx = 0; m_vCards.clear(); }
	
	virtual ~IPoker(){}
	
	virtual void init( Json::Value& jsOpts ) = 0;

	void shuffle()
	{
		uint8_t n = 0;
		for ( uint8_t i = 0; i < m_vCards.size(); ++i)
		{
			n = rand() % (uint8_t)m_vCards.size();
			if (n == i)
			{
				continue;
			}
			m_vCards[i] = m_vCards[n] + m_vCards[i];
			m_vCards[n] = m_vCards[i] - m_vCards[n];
			m_vCards[i] = m_vCards[i] - m_vCards[n];
		}
		m_nCurIdx = 0;
	}

	void pushCardToFron(uint8_t nCard)
	{
		std::size_t nFindIdx = -1;
		for (std::size_t nIdx = m_nCurIdx; nIdx < m_vCards.size(); ++nIdx)
		{
			if (nCard == m_vCards[nIdx])
			{
				nFindIdx = nIdx;
				break;
			}
		}

		if (nFindIdx == (std::size_t) - 1 || nFindIdx == m_nCurIdx )
		{
			return;
		}

		m_vCards[nFindIdx] = m_vCards[m_nCurIdx] + m_vCards[nFindIdx];
		m_vCards[m_nCurIdx] = m_vCards[nFindIdx] - m_vCards[m_nCurIdx];
		m_vCards[nFindIdx] = m_vCards[nFindIdx] - m_vCards[m_nCurIdx];
	}

	uint8_t getLeftCardCount()
	{
		return (uint8_t)m_vCards.size() - m_nCurIdx;
	}

	uint8_t distributeOneCard()
	{
		if ( m_vCards.size() <= m_nCurIdx)
		{
			shuffle();
		}
		return m_vCards[m_nCurIdx++];
	}

	static uint8_t makeCard( uint8_t nPartA, uint8_t nPartB )
	{
		uint8_t nCard = ( (nPartA << 4) | nPartB );
		return nCard;
	}

	static uint8_t parsePartA(uint8_t nCardNum)
	{
		return (nCardNum & 0xf0) >> 4;
	}

	static uint8_t parsePartB(uint8_t nCardNum)
	{
		return (nCardNum & 0x0f);
	}
protected:
	virtual void addCardToPoker( uint8_t nCard ) // only can invoker in , init method ;
	{
		m_vCards.push_back(nCard);
	}
protected:
	std::vector<uint8_t> m_vCards;
	uint16_t m_nCurIdx;
};