#include "BJPlayerCard.h"
#include "log4z.h"
#include <algorithm>
// stGround card 
void BJPlayerCard::stGroupCard::reset()
{
	m_eCardType = ePeerCard_None;
	m_pPairCardNum = 0;
}

void BJPlayerCard::stGroupCard::setCard(uint8_t nA, uint8_t nB, uint8_t nC)
{
	m_vCard[0].RsetCardByCompositeNum(nA);
	m_vCard[1].RsetCardByCompositeNum(nB);
	m_vCard[2].RsetCardByCompositeNum(nC);
	arrangeCard();
}

int8_t BJPlayerCard::stGroupCard::PKPeerCard(stGroupCard* pPeerCard)  // 1 win , 0 same , -1 failed 
{
	auto nSelf = getWeight();
	auto nT = pPeerCard->getWeight();

	if ( nSelf != nT )
	{
		return nSelf > nT ? 1 : -1;
	 }

	LOGFMTE( "why two card is the same ?  A = %u , %u , %u , B = %u , %u, %u",
		m_vCard[0].GetCardCompositeNum(), m_vCard[1].GetCardCompositeNum(), m_vCard[2].GetCardCompositeNum()
		,pPeerCard->m_vCard[0].GetCardCompositeNum(), pPeerCard->m_vCard[1].GetCardCompositeNum(), pPeerCard->m_vCard[2].GetCardCompositeNum());
	return 1;
}

uint32_t BJPlayerCard::stGroupCard::getWeight()
{
	uint32_t nWeight = 0;
	uint8_t nPairFace = 0;
	uint8_t nBigSingle = 0;

	bool isShunZi = (GetType() == ePeerCard_Sequence || ePeerCard_SameColorSequence == GetType());
	if (isShunZi) // 123 smallest shun zi ;
	{
		if (m_vCard[0].GetCardFaceNum() == 2 && m_vCard[1].GetCardFaceNum() == 3 && m_vCard[2].GetCardFaceNum() == 1)
		{
			decltype(m_vCard) temp = { m_vCard[2],m_vCard[0],m_vCard[1] };
			m_vCard.swap(temp);
		}
	}
	else
	{
		if ( ePeerCard_Pair == GetType() )
		{
			if ( m_vCard[0].GetCardFaceNum() == m_vCard[1].GetCardFaceNum())
			{
				nPairFace = m_vCard[0].GetCardFaceNum();
				nBigSingle = m_vCard[2].GetCardCompositeNum();
			}
			else
			{
				nBigSingle = m_vCard[0].GetCardCompositeNum();
				nPairFace = m_vCard[1].GetCardFaceNum();
			}
		}
	}

	if ( nBigSingle == 0 )
	{
		nBigSingle = m_vCard.back().GetCardCompositeNum();
	}

	nWeight = ( GetType() << 12 ) | ( nPairFace << 8 ) | nBigSingle;
	return nWeight;
}

void BJPlayerCard::stGroupCard::arrangeCard()
{
	for (uint8_t nIdx = 0; nIdx < PEER_CARD_COUNT - 1; ++nIdx)
	{
		uint8_t nPosNum = m_vCard[nIdx].GetCardFaceNum(true);
		for (uint8_t nSIdx = nIdx + 1; nSIdx < PEER_CARD_COUNT; ++nSIdx)
		{
			uint8_t nSNum = m_vCard[nSIdx].GetCardFaceNum(true);
			if (nSNum < nPosNum) // switch ;
			{
				nPosNum = nSNum;
				CCard& pCard = m_vCard[nSIdx];
				m_vCard[nSIdx] = m_vCard[nIdx];
				m_vCard[nIdx] = pCard;
			}
		}
	}

	int iNum[PEER_CARD_COUNT] = { 0 };
	for (int i = 0; i < PEER_CARD_COUNT; ++i)
	{
		iNum[i] = m_vCard[i].GetCardFaceNum(true);
	}

	// decide type ;
	if (m_vCard[0].GetCardFaceNum() == m_vCard[1].GetCardFaceNum() && m_vCard[1].GetCardFaceNum() == m_vCard[2].GetCardFaceNum())
	{
		m_eCardType = ePeerCard_Bomb;
	}
	else if (m_vCard[0].GetType() == m_vCard[1].GetType() && m_vCard[1].GetType() == m_vCard[2].GetType())
	{
		m_eCardType = ePeerCard_SameColor;
		if (iNum[0] + 1 == iNum[1] && iNum[1] + 1 == iNum[2])
		{
			m_eCardType = ePeerCard_SameColorSequence;
		}

		if (iNum[0] == 2 && iNum[1] == 3 && 14 == iNum[2])
		{
			m_eCardType = ePeerCard_SameColorSequence;
		}
	}
	else if ((iNum[0] + 1 == iNum[1] && iNum[1] + 1 == iNum[2]) || (iNum[0] == 2 && iNum[1] == 3 && 14 == iNum[2]))
	{
		m_eCardType = ePeerCard_Sequence;
	}
	else if (iNum[0] == iNum[1] || iNum[2] == iNum[1])
	{
		m_eCardType = ePeerCard_Pair;
		m_pPairCardNum = iNum[1];
	}
	else
	{
		m_eCardType = ePeerCard_None;
	}
}

uint8_t BJPlayerCard::stGroupCard::getCardByIdx( uint8_t nIdx )
{
	if ( nIdx >= PEER_CARD_COUNT )
	{
		LOGFMTE( "get card from group but invalid nidx = %u",nIdx );
		return 0;
	}
	return m_vCard[nIdx].GetCardCompositeNum();
}

// player card 
BJPlayerCard::BJPlayerCard()
{
	m_vGroups.resize(MAX_GROUP_CNT);
	m_vHoldCards.clear();
}

void BJPlayerCard::reset()
{
	m_vHoldCards.clear();
	m_nCurGroupIdx = 0;
}

void BJPlayerCard::addCompositCardNum(uint8_t nCardCompositNum)
{
	if ( m_vHoldCards.size() >= 9 )
	{
		LOGFMTE( "hold card is overflow , can not add more" );
		return;
	}

	m_vHoldCards.push_back(nCardCompositNum);
}

const char* BJPlayerCard::getNameString()
{
	return "default";
}

uint32_t BJPlayerCard::getWeight()
{
	return m_vGroups[getCurGroupIdx()].getWeight();
}

IPeerCard* BJPlayerCard::swap(IPeerCard* pTarget)
{
	auto pT = (BJPlayerCard*)pTarget;
	m_vHoldCards.swap(pT->m_vHoldCards);
	m_vGroups.swap(pT->m_vGroups);
	return pTarget;
}

uint8_t BJPlayerCard::getCardByIdx(uint8_t nidx)
{
	return m_vGroups[getCurGroupIdx()].getCardByIdx(nidx);
}

bool BJPlayerCard::getGroupInfo( uint8_t nGroupIdx, uint8_t& nGroupType, std::vector<uint8_t>& vGroupCards)
{
	if (nGroupIdx >= MAX_GROUP_CNT)
	{
		LOGFMTE("invalid group idx = %u, can not get info", nGroupIdx );
		return false;
	}

	auto& pGInfo = m_vGroups[nGroupIdx];
	nGroupType = pGInfo.GetType();
	for ( uint8_t nIdx = 0; nIdx < PEER_CARD_COUNT; ++nIdx )
	{
		vGroupCards.push_back(pGInfo.getCardByIdx(nIdx));
	}
	return true;
}

void BJPlayerCard::setCurGroupIdx(uint8_t nGrpIdx)
{
	if (nGrpIdx >= MAX_GROUP_CNT)
	{
		LOGFMTE( "why you set a invalid group idx = %u",nGrpIdx );
		return;
	}

	m_nCurGroupIdx = nGrpIdx;
}

uint8_t BJPlayerCard::getCurGroupIdx()
{
	return m_nCurGroupIdx;
}

bool BJPlayerCard::setCardsGroup(std::vector<uint8_t>& vGroupedCards)
{
	if (vGroupedCards.empty())
	{
		// auto make group ;
		return true;
	}

	if ( vGroupedCards.size() != m_vHoldCards.size() || vGroupedCards.size() != 9 )
	{
		LOGFMTE( "group cards is not equal" );
		return false;
	}

	// check group cards is valid 
	auto vTemp = vGroupedCards;
	std::sort(vTemp.begin(),vTemp.end());
	std::sort(m_vHoldCards.begin(), m_vHoldCards.end());
	for ( uint8_t nIdx = 0; nIdx < vTemp.size(); ++nIdx )
	{
		if ( vTemp[nIdx] != m_vHoldCards[nIdx] )
		{
			return false;
		}
	}

	m_vHoldCards.clear();
	m_vHoldCards = vGroupedCards;

	// fill group 
	for ( uint8_t nGIdx = 0; nGIdx < m_vGroups.size(); ++nGIdx )
	{
		m_vGroups[nGIdx].setCard(m_vHoldCards[3 * nGIdx], m_vHoldCards[3 * nGIdx] + 1, m_vHoldCards[3 * nGIdx] + 2 );
	}
	return true;
}

bool BJPlayerCard::holdCardToJson(Json::Value& vHoldCards)
{
	for ( auto& ref : m_vHoldCards )
	{
		vHoldCards[vHoldCards.size()] = ref;
	}
	return true;
}