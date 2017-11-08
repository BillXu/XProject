#include "DDZPlayerCard.h"
#include "DDZFindCardTypeForChu.h"
void DDZPlayerCard::reset()
{
	m_vHoldCards.clear();
	m_vLastChu.clear();
	m_nChuedCardTimes = 0;
}

void DDZPlayerCard::addHoldCard(uint8_t nCard)
{
	m_vHoldCards.push_back(nCard);
}

bool DDZPlayerCard::onChuCard(std::vector<uint8_t>& vCards)
{
	std::vector<uint8_t> vErased;
	for (auto& ref : vCards)
	{
		auto iter = std::find(m_vHoldCards.begin(),m_vHoldCards.end(),ref);
		if ( iter == m_vHoldCards.end())
		{
			break;
		}
		m_vHoldCards.erase(iter);
		vErased.push_back(ref);
	}

	if ( vErased.size() != vCards.size() )
	{
		m_vHoldCards.insert(m_vHoldCards.end(),vErased.begin(),vErased.end());
		return false;
	}
	m_vLastChu = vErased;
	++m_nChuedCardTimes;
	return true;
}

void DDZPlayerCard::holdCardToJson(Json::Value& jsHoldCard)
{
	for (auto& ref : m_vHoldCards)
	{
		jsHoldCard[jsHoldCard.size()] = ref;
	}
	return;
}

void DDZPlayerCard::lastChuToJson(Json::Value& jsHoldCard)
{
	for (auto& ref : m_vLastChu )
	{
		jsHoldCard[jsHoldCard.size()] = ref;
	}
	return;
}

void DDZPlayerCard::clearLastChu()
{
	m_vLastChu.clear();
}

uint8_t DDZPlayerCard::getHoldCardCount()
{
	return m_vHoldCards.size();
}

uint16_t DDZPlayerCard::getChuedCardTimes()
{
	return m_nChuedCardTimes;
}

bool DDZPlayerCard::isHaveCard(uint8_t nCard)
{
	return std::find(m_vHoldCards.begin(), m_vHoldCards.end(), nCard) != m_vHoldCards.end();
}

bool DDZPlayerCard::getTuoGuanChuCards(DDZ_Type& nCurAndOutType, std::vector<uint8_t>& vCmpCards, std::vector<uint8_t>& vResultCards)
{
	return DDZFindTuoGuanChuCard::getInstance()->findCardToChu(m_vHoldCards, nCurAndOutType, vCmpCards, vResultCards);
}