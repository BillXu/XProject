#include "BJPlayerCard.h"
#include "log4z.h"
#include <algorithm>
#include "BJCardTypeChecker.h"
#include "BJXiPaiChecker.h"
// stGround card 
void BJPlayerCard::stGroupCard::reset()
{
	m_eCardType = CardType_None;
	m_nWeight = 0;
	m_vCard.clear();
}

void BJPlayerCard::stGroupCard::setCard( std::vector<uint16_t>& vCompsitCard )
{
	m_vCard.clear();
	m_vCard.assign(vCompsitCard.begin(),vCompsitCard.end());
	if (m_vCard.size() != 3)
	{
		LOGFMTE( "card size is not 3 , why ? big error " );
		return;
	}

	// get real caculate card ;
	std::vector<uint8_t> vec;
	for (auto& ref : m_vCard)
	{
		uint16_t nValue = ref & 0xff;
		vec.push_back((uint8_t)nValue);
	}

	auto bRet = BJCardTypeChecker::getInstance()->checkCardType(vec, m_nWeight, m_eCardType);
	if ( bRet == false )
	{
		LOGFMTE( "why check card type return error ? must check it !!!" );
	}
	return;
}

uint16_t BJPlayerCard::stGroupCard::getCardByIdx( uint8_t nIdx )
{
	if ( nIdx >= PEER_CARD_COUNT || m_vCard.size() <= nIdx )
	{
		LOGFMTE( "get card from group but invalid nidx = %u",nIdx );
		return 0;
	}
	return m_vCard[nIdx];
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
	for (auto& ref : m_vGroups)
	{
		ref.reset();
	}
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
	return m_vHoldCards[nidx];
}

bool BJPlayerCard::getGroupInfo( uint8_t nGroupIdx, uint8_t& nGroupType, std::vector<uint16_t>& vGroupCards)
{
	if (nGroupIdx >= MAX_GROUP_CNT)
	{
		LOGFMTE("invalid group idx = %u, can not get info", nGroupIdx );
		return false;
	}

	auto& pGInfo = m_vGroups[nGroupIdx];
	nGroupType = pGInfo.getType();
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

bool BJPlayerCard::setCardsGroup(std::vector<uint16_t>& vGroupedCards)
{
	if (vGroupedCards.empty())
	{
		// auto make group ;
		autoChoseGroup(vGroupedCards);
	}

	if ( vGroupedCards.size() != m_vHoldCards.size() || vGroupedCards.size() != 9 )
	{
		LOGFMTE( "group cards is not equal" );
		return false;
	}

	// check group cards is valid 
	std::vector<uint8_t> vTemp;
	for (auto ref : vGroupedCards)
	{
		uint8_t nValue = (uint8_t)( ref & 0xff );
		if ( ref & 0xff00)
		{
			nValue = (uint8_t)((ref & 0xff00) >> 8 );
		}
		vTemp.push_back(nValue);
	}
	std::sort(vTemp.begin(),vTemp.end());
	std::sort(m_vHoldCards.begin(), m_vHoldCards.end());
	for ( uint8_t nIdx = 0; nIdx < vTemp.size(); ++nIdx )
	{
		if ( vTemp[nIdx] != m_vHoldCards[nIdx] )
		{
			LOGFMTE( "recived client card is not the same as server , client fake" );
			return false;
		}
	}

	// fill group 
	auto groupIter = vGroupedCards.begin();
	for ( uint8_t nGIdx = 0; nGIdx < m_vGroups.size(); ++nGIdx )
	{
		std::vector<uint16_t> vTemp;
		vTemp.push_back(*groupIter);
		++groupIter;
		vTemp.push_back(*groupIter);
		++groupIter;
		vTemp.push_back(*groupIter);
		++groupIter;

		m_vGroups[nGIdx].setCard(vTemp);
	}
	return true;
}

void BJPlayerCard::autoChoseGroup(std::vector<uint16_t>& vGroupedCards)
{

}

bool BJPlayerCard::holdCardToJson(Json::Value& vHoldCards)
{
	for ( auto& ref : m_vHoldCards )
	{
		vHoldCards[vHoldCards.size()] = ref;
	}
	return true;
}

bool BJPlayerCard::groupCardToJson(Json::Value& vHoldCards)
{
	for (auto& ref : m_vGroups)
	{
		vHoldCards[vHoldCards.size()] = ref.getCardByIdx(0);
		vHoldCards[vHoldCards.size()] = ref.getCardByIdx(1);
		vHoldCards[vHoldCards.size()] = ref.getCardByIdx(2);
	}
	return true;
}

void BJPlayerCard::getHoldCards(std::vector<uint8_t>& vHoldCards)
{
	vHoldCards.assign(m_vHoldCards.begin(),m_vHoldCards.end());
}

BJPlayerCard::stGroupCard& BJPlayerCard::getGroupByIdx(uint8_t nGroupIdx)
{
	if (nGroupIdx >= 3)
	{
		LOGFMTE( "invalid groupidx = %u, must fix it",nGroupIdx );
		nGroupIdx = 0;
	}
	return m_vGroups[nGroupIdx];
}

eXiPaiType BJPlayerCard::getXiPaiType(bool isEnableSanQing, bool isEnableShunQingDaTou)
{
	eXiPaiType temp;
	auto bRet = BJXiPaiChecker::getInstance()->checkXiPai(this, temp, isEnableSanQing, isEnableShunQingDaTou);
	if (bRet)
	{
		temp = eXiPai_Max;
	}
	return temp;
}