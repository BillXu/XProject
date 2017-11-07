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

void BJPlayerCard::stGroupCard::setCard( std::vector<uint8_t>& vCompsitCard )
{
	m_vCard.clear();
	m_vCard.assign(vCompsitCard.begin(),vCompsitCard.end());
	if (m_vCard.size() != 3)
	{
		LOGFMTE( "card size is not 3 , why ? big error " );
		return;
	}

	auto bRet = BJCardTypeChecker::getInstance()->checkCardType(vCompsitCard, m_nWeight, m_eCardType);
	if ( bRet == false )
	{
		LOGFMTE( "why check card type return error ? must check it !!!" );
	}
	return;
}

uint8_t BJPlayerCard::stGroupCard::getCardByIdx( uint8_t nIdx )
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

bool BJPlayerCard::getGroupInfo( uint8_t nGroupIdx, uint8_t& nGroupType, std::vector<uint8_t>& vGroupCards)
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

bool BJPlayerCard::setCardsGroup(std::vector<uint8_t>& vGroupedCards)
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
	std::vector<uint8_t> vTemp = vGroupedCards;
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
		std::vector<uint8_t> vTemp;
		vTemp.push_back(*groupIter);
		++groupIter;
		vTemp.push_back(*groupIter);
		++groupIter;
		vTemp.push_back(*groupIter);
		++groupIter;

		m_vGroups[nGIdx].setCard(vTemp);
	}

	std::sort(m_vGroups.begin(), m_vGroups.end(), [](stGroupCard& a, stGroupCard& b) { return a.getWeight() < a.getWeight(); });
	
	for (auto& ref : m_vGroups)
	{

	}
	return true;
}

void BJPlayerCard::autoChoseGroup(std::vector<uint8_t>& vGroupedCards)
{
	std::vector<uint8_t> vLeftHold = m_vHoldCards;

	std::vector<uint8_t> vMax;
	std::vector<uint8_t> vCheck;
	uint32_t nTmpWeight = 0;
	
	do
	{
		vCheck.clear();
		vMax.clear();
		nTmpWeight = 0;

		findMaxCards(vLeftHold, vCheck, vMax, nTmpWeight);
		if (vMax.size() == 0)
		{
			LOGFMTE("why can not find max group ?");
			vGroupedCards = m_vHoldCards;
			return;
		}

		vGroupedCards.insert(vGroupedCards.end(), vMax.begin(), vMax.end());
		for (auto& ref : vMax)
		{
			auto iter = std::find(vLeftHold.begin(), vLeftHold.end(),ref);
			vLeftHold.erase(iter);
		}

	} while ( vLeftHold.size() >= 3 );
}

bool BJPlayerCard::findMaxCards( std::vector<uint8_t>& vLeftWaitCheck, std::vector<uint8_t>& vCheckCards, std::vector<uint8_t>& vCurMax, uint32_t& nCurMaxWeight)
{
	if ( vCheckCards.size() == 3 )
	{
		uint32_t nWeightTmp = 0;
		eBJCardType type;
		BJCardTypeChecker::getInstance()->checkCardType(vCheckCards, nWeightTmp, type );
		if (nWeightTmp > nCurMaxWeight)
		{
			nCurMaxWeight = nWeightTmp;
			vCurMax = vCheckCards;
		}
		return true;
	}

	for ( uint8_t nIdx = 0; nIdx < vLeftWaitCheck.size(); ++nIdx)
	{
		vCheckCards.push_back(vLeftWaitCheck[nIdx]);
		std::vector<uint8_t> vLeft;
		for (uint8_t nLeft = nIdx + 1; nLeft < vLeftWaitCheck.size(); ++nLeft)
		{
			vLeft.push_back(vLeftWaitCheck[nLeft]);
		}

		uint32_t nWeitTemp = 0 ;
		std::vector<uint8_t> vTmp;
		findMaxCards(vLeft,vCheckCards,vTmp, nWeitTemp);
		if ( nWeitTemp > nCurMaxWeight )
		{
			nCurMaxWeight = nWeitTemp;
			vCurMax = vTmp;
		}
		vCheckCards.pop_back();
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

uint8_t BJPlayerCard::getXiPaiType(bool isEnableSanQing, bool isEnableShunQingDaTou, std::vector<eXiPaiType>& vXiType )
{
	auto bRet = BJXiPaiChecker::getInstance()->checkXiPai(this, vXiType, isEnableSanQing, isEnableShunQingDaTou);
	if (!bRet)
	{
		vXiType.clear();
		return 0;
	}
	
	// remove duplicate ; shun qing types
	auto iter = std::find(vXiType.begin(),vXiType.end(),eXiPai_QuanShunQing );
	if ( iter != vXiType.end() )
	{
		auto iterShuang = std::find(vXiType.begin(), vXiType.end(), eXiPai_ShuangShunQing );
		if ( iterShuang != vXiType.end() )
		{
			vXiType.erase(iterShuang);
		}

		auto iterShunQing = std::find(vXiType.begin(), vXiType.end(), eXiPai_ShunQingDaTou );
		if (iterShunQing != vXiType.end())
		{
			vXiType.erase(iterShunQing);
		}
	}

	iter = std::find(vXiType.begin(), vXiType.end(), eXiPai_ShuangShunQing );
	if (iter != vXiType.end())
	{
		auto iterShunQing = std::find(vXiType.begin(), vXiType.end(), eXiPai_ShunQingDaTou);
		if (iterShunQing != vXiType.end())
		{
			vXiType.erase(iterShunQing);
		}
	}

	// san tiao type
	iter = std::find(vXiType.begin(), vXiType.end(), eXiPai_QuanSanTiao );
	if (iter != vXiType.end())
	{
		auto iterShunQing = std::find(vXiType.begin(), vXiType.end(), eXiPai_ShuangSanTiao);
		if (iterShunQing != vXiType.end())
		{
			vXiType.erase(iterShunQing);
		}
	}

	// caculate rate 
	uint8_t nBeiShu = 0;
	auto iter3QuanS = std::find(vXiType.begin(), vXiType.end(), eXiPai_QuanShunQing);
	auto iter3QuanST = std::find(vXiType.begin(), vXiType.end(), eXiPai_QuanSanTiao);
	if ( iter3QuanS != vXiType.end() || iter3QuanST != vXiType.end())
	{
		nBeiShu = 2;
	}

	auto iterShuangSiZhang = std::find(vXiType.begin(), vXiType.end(), eXiPai_ShuangSiZhang );
	if (iterShuangSiZhang != vXiType.end())
	{
		nBeiShu += 1;
	}
	nBeiShu += vXiType.size();
	return nBeiShu;
}