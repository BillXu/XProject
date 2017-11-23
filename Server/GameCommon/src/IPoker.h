#pragma once
#include "NativeTypes.h"
#include "json\json.h"
#include <algorithm>
class IPoker
{
public:
	IPoker() { m_nCurIdx = 0; m_vCards.clear(); }
	
	virtual ~IPoker(){}
	
	virtual void init( Json::Value& jsOpts ) = 0;

	void shuffle()
	{
		std::random_shuffle(m_vCards.begin(), m_vCards.end());
		m_nCurIdx = 0;
#ifdef _DEBUG
		std::vector<uint8_t> vOutCards;
		makeSpecialCard(vOutCards);
		for (auto& ref : vOutCards)
		{
			auto iter = std::find(m_vCards.begin(),m_vCards.end(),ref);
			if (iter != m_vCards.end())
			{
				m_vCards.erase(iter);
			}
		}
		m_vCards.insert(m_vCards.begin(),vOutCards.begin(),vOutCards.end());
#endif
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
		uint8_t nCard = ( (nPartA << 3) | nPartB );
		return nCard;
	}

	static uint8_t parsePartA(uint8_t nCardNum)
	{
		return (nCardNum & 0xf8) >> 3;
	}

	static uint8_t parsePartB(uint8_t nCardNum)
	{
		return (nCardNum & 0x07);
	}
protected:
	virtual void addCardToPoker( uint8_t nCard ) // only can invoker in , init method ;
	{
		m_vCards.push_back(nCard);
	}

	virtual void makeSpecialCard( std::vector<uint8_t>& vMakedCards ){}
protected:
	std::vector<uint8_t> m_vCards;
	uint16_t m_nCurIdx;
};