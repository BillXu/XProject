#include "GDPlayerCard.h"
#include "CardPoker.h"
void GDPlayerCard::reset()
{
	m_vHoldCards.clear();
	clearLastChu();
	m_nChuedCardTimes = 0;
}

void GDPlayerCard::addHoldCard(uint8_t nCard)
{
	m_vHoldCards.push_back(nCard);
}

bool GDPlayerCard::onPayCard(uint8_t nCard) {
	auto iter = std::find(m_vHoldCards.begin(), m_vHoldCards.end(), nCard);
	if (iter == m_vHoldCards.end())
	{
		return false;
	}
	m_vHoldCards.erase(iter);
	return true;
}

bool GDPlayerCard::onChuCard(std::vector<uint8_t>& vCards, GD_Type nType)
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
	m_nLastChuType = nType;
	++m_nChuedCardTimes;
	return true;
}

void GDPlayerCard::holdCardToJson(Json::Value& jsHoldCard)
{
	for (auto& ref : m_vHoldCards)
	{
		jsHoldCard[jsHoldCard.size()] = ref;
	}
	return;
}

bool GDPlayerCard::lastChuToJson(Json::Value& jsHoldCard)
{
	if (m_nLastChuType > GD_None) {
		Json::Value jsCards;
		for (auto& ref : m_vLastChu)
		{
			jsCards[jsCards.size()] = ref;
		}
		jsHoldCard["chu"] = jsCards;
		jsHoldCard["type"] = m_nLastChuType;
		return true;
	}
	return false;
}

void GDPlayerCard::clearLastChu()
{
	m_vLastChu.clear();
	m_nLastChuType = GD_None;
}

void GDPlayerCard::getHoldCard(std::vector<uint8_t>& vHoldCards)
{
	vHoldCards.insert(vHoldCards.end(), m_vHoldCards.begin(), m_vHoldCards.end());
}

uint8_t GDPlayerCard::getHoldCardCount()
{
	return m_vHoldCards.size();
}

uint16_t GDPlayerCard::getChuedCardTimes()
{
	return m_nChuedCardTimes;
}

bool GDPlayerCard::isHaveCard(uint8_t nCard)
{
	return std::find(m_vHoldCards.begin(), m_vHoldCards.end(), nCard) != m_vHoldCards.end();
}

bool GDPlayerCard::isCardInvalidPayTribute(uint8_t nCard, uint8_t nDaJi) {
	if (isHaveCard(nCard) == false) {
		return false;
	}

	auto nBiggestCard = getBiggestCardExcept(nDaJi);
	if (GD_PARSE_VALUE(nCard) != GD_PARSE_VALUE(nBiggestCard)) {
		return false;
	}

	if (GD_PARSE_VALUE(nCard) == nDaJi && GD_PARSE_TYPE(nCard) == ePoker_Heart) {
		return false;
	}

	return true;
}

bool GDPlayerCard::isCardInvalidBackTribute(uint8_t nCard, uint8_t nDaJi) {
	if (isHaveCard(nCard) == false) {
		return false;
	}

	auto nValue = GD_PARSE_VALUE(nCard);

	if (nValue > 10 || nValue == nDaJi) {
		return false;
	}

	return true;
}

uint8_t GDPlayerCard::getBiggestCardExcept(uint8_t nDaJi) {
	std::vector<uint8_t> nHoldCards;
	getHoldCard(nHoldCards);

	for (auto& ref : nHoldCards) {
		if (GD_PARSE_VALUE(ref) == nDaJi) {
			ref = GD_MAKE_CARD(GD_PARSE_TYPE(ref), 16);
		}
	}

	std::sort(nHoldCards.begin(), nHoldCards.end(), [](const uint8_t& a, const uint8_t& b) {
		return a > b;
	});

	uint8_t nCard = 0;
	for (auto ref : m_vHoldCards) {
		auto ref_value = GD_PARSE_VALUE(ref);
		auto ref_type = GD_PARSE_TYPE(ref);
		if (ref_value == 16 && ref_type == ePoker_Heart) {
			continue;
		}

		if (ref_value == 16) {
			ref_value = nDaJi;
		}

		nCard = GD_MAKE_CARD(ref_type, ref_value);
	}

	return nCard;
}

uint8_t GDPlayerCard::getSmallestCardExcept(uint8_t nDaJi) {
	std::vector<uint8_t> nHoldCards;
	getHoldCard(nHoldCards);

	for (auto& ref : nHoldCards) {
		if (GD_PARSE_VALUE(ref) == nDaJi) {
			ref = GD_MAKE_CARD(GD_PARSE_TYPE(ref), 16);
		}
	}

	std::sort(nHoldCards.begin(), nHoldCards.end(), [](const uint8_t& a, const uint8_t& b) {
		return a < b;
	});

	uint8_t nCard = 0;
	for (auto ref : m_vHoldCards) {
		auto ref_value = GD_PARSE_VALUE(ref);
		auto ref_type = GD_PARSE_TYPE(ref);
		if (ref_value == 16 && ref_type == ePoker_Heart) {
			continue;
		}

		if (ref_value == 16) {
			ref_value = nDaJi;
		}

		nCard = GD_MAKE_CARD(ref_type, ref_value);
	}

	return nCard;
}

uint8_t GDPlayerCard::autoGetPayTributeCard(uint8_t nDaJi) {
	return getBiggestCardExcept(nDaJi);
}

uint8_t GDPlayerCard::autoGetBackTributeCard(uint8_t nDaJi) {
	return getSmallestCardExcept(nDaJi);
}