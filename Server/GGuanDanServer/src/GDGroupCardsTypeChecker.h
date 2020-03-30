#pragma once
#include "Singleton.h"
#include <algorithm>
class IGDGroupCardsType
{
public:
	virtual bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, GD_Type& cardType, uint8_t nDaJi) = 0;
	virtual GD_Type getThisCardType() = 0;
};

/*
重要面值1（4） | 重要面值2（4）| 重要面值3（4）| 重要牌花色类型(4)， 总共32位
A>K>Q>J>10>9...>2
黑桃>红桃>梅花>方块
对子+单张，同花，顺子比最大单张花色
1,2,3,4,5 是最小的顺子 比A花色
头道3张牌只有单张，对子，三张
*/

class GDGroupCardsTypeJokerBomb
	:public IGDGroupCardsType
{
public:
	GD_Type getThisCardType()override {
		return GD_Rokect;
	}

	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, GD_Type& cardType, uint8_t nDaJi)override
	{
		if (vecCards.size() != 4) {
			return false;
		}
		uint8_t jokerCnt = std::count(vecCards.begin(), vecCards.end(), GD_MAKE_CARD(ePoker_Joker, 18));
		if (jokerCnt < 2) {
			return false;
		}
		jokerCnt = std::count(vecCards.begin(), vecCards.end(), GD_MAKE_CARD(ePoker_Joker, 19));
		if (jokerCnt < 2) {
			return false;
		}
		cardType = GD_Rokect;
		nWeight = 0;
		return true;
	}
};

/*
weight : 单张最大面值 + 花色
*/
class GDGroupCardsTypeSixBomb
	:public IGDGroupCardsType
{
public:
	GD_Type getThisCardType()override {
		return GD_Bomb6OrMore;
	}

	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, GD_Type& cardType, uint8_t nDaJi)override
	{
		if (vecCards.size() < 6) {
			return false;
		}

		uint8_t tValue = 0;
		for (auto ref : vecCards) {
			if (GD_PARSE_VALUE(ref) == nDaJi && GD_PARSE_TYPE(ref) == ePoker_Heart) {
				continue;
			}
			if (tValue) {
				if (tValue == GD_PARSE_VALUE(ref)) {
					continue;
				}
				return false;
			}
			else {
				tValue = GD_PARSE_VALUE(ref);
			}
		}

		nWeight = tValue;
		cardType = GD_Bomb6OrMore;
		return true;
	}
};

class GDGroupCardsTypeFlush
	:public IGDGroupCardsType
{
public:
	GD_Type getThisCardType()override {
		return GD_Flush5;
	}

	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, GD_Type& cardType, uint8_t nDaJi)override
	{
		if (vecCards.size() != 5) {
			return false;
		}

	}

protected:
	bool isFlash(std::vector<uint8_t>& vecCards, uint8_t nDaJi) {
		uint8_t nFlush = ePoker_None;
		for (auto& ref : vecCards) {
			auto nValue = GD_PARSE_VALUE(ref);
			auto nType = GD_PARSE_TYPE(ref);
			if (nValue == nDaJi && nType == ePoker_Heart) {
				continue;
			}
			if (nFlush) {
				if (nFlush != nType) {
					return false;
				}
			}
			else {
				nFlush = nType;
			}
		}
		return true;
	}

	bool isStraight(std::vector<uint8_t>& vecCards, uint8_t nDaJi) {
		uint8_t nBaiDa = 0;
		std::vector<uint8_t> vValues;
		bool bHasA = false;
		for (auto ref : vecCards) {
			auto nValue = GD_PARSE_VALUE(ref);
			auto nType = GD_PARSE_TYPE(ref);
			if (nValue == nDaJi && nType == ePoker_Heart) {
				nBaiDa++;
			}
			else {
				vValues.push_back(nValue);
				if (nValue == 14) {
					bHasA = true;
				}
			}
		}

		if (bHasA) {
			bool isA14 = std::find_if(vValues.begin(), vValues.end(), [nDaJi](const uint8_t& tValue) {
				return tValue < 10;
			}) == vValues.end();

			if (isA14 == false) {
				for (auto& ref : vValues) {
					if (ref == 14) {
						ref = 1;
					}
				}
			}
		}

		std::sort(vValues.begin(), vValues.end(), [](const uint8_t& a, const uint8_t& b) {
			return a > b;
		});
		uint8_t nValue = 0;
		for (auto ref : vValues) {
			if (nValue) {
				
			}
			else {
				nValue = ref;
			}
		}
	}
};

class GDGroupCardsTypeThreeStraightBySingle
	:public IGDGroupCardsType
{
public:
	GD_Type getThisCardType()override {
		return eGDCardType_ThreeStraightBySingle;
	}

	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, GD_Type& cardType)override
	{
		if (vecCards.size() < 5 || vecCards.size() % 4) {
			return false;
		}
		std::map<uint8_t, uint8_t> mCardCnt;
		for (auto ref : vecCards) {
			auto tValue = DDZ_PARSE_VALUE(ref);
			if (mCardCnt.count(tValue)) {
				continue;
			}
			else {
				uint8_t tCnt = std::count_if(vecCards.begin(), vecCards.end(), [tValue](uint8_t& nCard) {
					return DDZ_PARSE_VALUE(nCard) == tValue;
				});
				mCardCnt[tValue] = tCnt;
			}
		}

		uint8_t nMaxStraightValue = 0;
		uint8_t nStraightCnt = 0;
		if (canBeThreeStraightBySignle(mCardCnt, 0, nMaxStraightValue, nStraightCnt, 0, false)) {
			cardType = eGDCardType_ThreeStraightBySingle;
			nWeight = (nStraightCnt << 4) | nMaxStraightValue;
			return true;
		}
		return false;
	}

protected:
	bool canBeThreeStraightBySignle(std::map<uint8_t, uint8_t> mCardCnt, uint8_t nKey, uint8_t& nStraigntKey, uint8_t& nStraightCnt, uint8_t nSingleCnt, bool canOnlySingle) {
		//先找本次验证的KEY
		for (auto ref : mCardCnt) {
			if (ref.first > nKey) {
				nKey = ref.first;
				break;
			}
		}

		//是否为最后一个key了
		auto itMap = mCardCnt.end();
		itMap--;
		if (nKey == itMap->first) {
			if (canOnlySingle == false
				&& nKey < 15
				&& nKey - nStraigntKey == 1
				&& itMap->second > 2
				&& nStraightCnt + 1 == nSingleCnt + (itMap->second - 3)) {
				nStraigntKey = nKey;
				nStraightCnt = nStraightCnt + 1;
				return true;
			}

			//成单统一处理
			return nStraightCnt == nSingleCnt + itMap->second;
		}

		//是否为第一个
		itMap = mCardCnt.begin();
		if (nKey == itMap->first) {
			if (itMap->second > 2) {
				nStraigntKey = nKey;
				nStraightCnt = 1;
				if (canBeThreeStraightBySignle(mCardCnt, nKey, nStraigntKey, nStraightCnt, itMap->second - 3, false)) {
					return true;
				}
			}

			//成单统一处理
			nStraigntKey = 0;
			nStraightCnt = 0;
			return canBeThreeStraightBySignle(mCardCnt, nKey, nStraigntKey, nStraightCnt, itMap->second, false);
		}

		//正常依次检查
		uint8_t nLastStraightKey = nStraigntKey;
		uint8_t nLastStraightCnt = nStraightCnt;
		itMap = mCardCnt.find(nKey);
		if (canOnlySingle == false
			&& nKey < 15
			&& itMap->second > 2
			&& (nStraigntKey == 0 || nKey - nStraigntKey == 1)) {
			nStraigntKey = nKey;
			nStraightCnt++;
			if (canBeThreeStraightBySignle(mCardCnt, nKey, nStraigntKey, nStraightCnt, nSingleCnt + (itMap->second - 3), false)) {
				return true;
			}
		}

		//成单统一处理
		nStraigntKey = nLastStraightKey;
		nStraightCnt = nLastStraightCnt;
		if (nStraigntKey
			&& nStraightCnt > 1
			&& canBeThreeStraightBySignle(mCardCnt, nKey, nStraigntKey, nStraightCnt, nSingleCnt + itMap->second, true)) {
			return true;
		}
		else {
			if (nStraigntKey || nStraightCnt) {
				return false;
			}
			else {
				return canBeThreeStraightBySignle(mCardCnt, nKey, nStraigntKey, nStraightCnt, nSingleCnt + itMap->second, false);
			}
		}
	}
};

class GDGroupCardsTypeThreeStraight
	:public IGDGroupCardsType
{
public:
	GD_Type getThisCardType()override {
		return eGDCardType_ThreeStraight;
	}

	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, GD_Type& cardType)override
	{
		if (vecCards.size() < 6 || vecCards.size() % 3) {
			return false;
		}
		uint8_t nValue = 0;
		for (auto ref : vecCards) {
			auto tValue = DDZ_PARSE_VALUE(ref);
			if (tValue > 14) {
				return false;
			}
			if (nValue == tValue) {
				continue;
			}
			else {
				if (nValue) {
					if (nValue - tValue != 1) {
						return false;
					}
				}
				nValue = tValue;
				uint8_t tCnt = std::count_if(vecCards.begin(), vecCards.end(), [tValue](uint8_t& nCard) {
					return DDZ_PARSE_VALUE(nCard) == tValue;
				});
				if (tCnt != 3) {
					return false;
				}
			}
		}
		cardType = eGDCardType_ThreeStraight;
		uint8_t nCnt = vecCards.size() / 3;
		nWeight = (nCnt << 4) | DDZ_PARSE_VALUE(vecCards[0]);
		return true;
	}
};

/*
weight : 最大一张牌面值 + 5张牌从大到小一次的花色
*/
class GDGroupCardsTypeDoubleStraight
	:public IGDGroupCardsType
{
public:
	GD_Type getThisCardType()override {
		return eGDCardType_DoubleStraight;
	}

	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, GD_Type& cardType)override
	{
		if (vecCards.size() < 6 || vecCards.size() % 2) {
			return false;
		}
		uint8_t nValue = 0;
		for (auto ref : vecCards) {
			auto tValue = DDZ_PARSE_VALUE(ref);
			if (tValue > 14) {
				return false;
			}
			if (nValue == tValue) {
				continue;
			}
			else {
				if (nValue) {
					if (nValue - tValue != 1) {
						return false;
					}
				}
				nValue = tValue;
				uint8_t tCnt = std::count_if(vecCards.begin(), vecCards.end(), [tValue](uint8_t& nCard) {
					return DDZ_PARSE_VALUE(nCard) == tValue;
				});
				if (tCnt != 2) {
					return false;
				}
			}
		}
		cardType = eGDCardType_DoubleStraight;
		uint8_t nCnt = vecCards.size() / 2;
		nWeight = (nCnt << 4) | DDZ_PARSE_VALUE(vecCards[0]);
		return true;
	}
};

/*
weight : 3张的牌面值 + 单张面值 + 大单张花色
*/
class GDGroupCardsTypeStraight
	:public IGDGroupCardsType
{
public:
	GD_Type getThisCardType()override {
		return eGDCardType_Straight;
	}

	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, GD_Type& cardType)override
	{
		if (vecCards.size() < 5) {
			return false;
		}
		uint8_t nValue = 0;
		for (auto ref : vecCards) {
			auto tValue = DDZ_PARSE_VALUE(ref);
			if (tValue > 14) {
				return false;
			}
			if (nValue == tValue) {
				return false;
			}
			if (nValue) {
				if (nValue - tValue != 1) {
					return false;
				}
			}
			nValue = tValue;
		}
		cardType = eGDCardType_Straight;
		uint8_t nCnt = vecCards.size();
		nWeight = (nCnt << 4) | DDZ_PARSE_VALUE(vecCards[0]);
		return true;
	}
};

/*
weight : 大对子牌面 + 小对子牌面 + 单张牌面 + 大单张花色
*/
class GDGroupCardsTypeFourByDouble
	:public IGDGroupCardsType
{
public:
	GD_Type getThisCardType()override {
		return eGDCardType_FourByDouble;
	}

	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, GD_Type& cardType)override
	{
		if (vecCards.size() != 8) {
			return false;
		}

		uint8_t nFind4 = 0;
		uint8_t nCnt = 0;
		uint8_t nPairCnt = 0;
		for (uint8_t i = 0; i < vecCards.size(); i++) {
			nCnt++;
			auto tValue = DDZ_PARSE_VALUE(vecCards[i]);
			if (i + 1 < vecCards.size() && tValue == DDZ_PARSE_VALUE(vecCards[i + 1])) {
				continue;
			}
			if (nCnt % 2) {
				return false;
			}
			if (nFind4 == 0 && nCnt > 3) {
				nFind4 = tValue;
				nPairCnt += (nCnt - 4) / 2;
			}
			else {
				nPairCnt += nCnt / 2;
			}
			nCnt = 0;
			if (nPairCnt > 2) {
				return false;
			}
		}

		if (nFind4 && nPairCnt == 2) {
			cardType = eGDCardType_FourByDouble;
			nWeight = nFind4;
			return true;
		}
		return false;
	}
};

/*
weight : 对子面值(4) + 单张从大到小一次面值(4) + 大单张花色(3)
*/
class GDGroupCardsTypeFourBySingle
	:public IGDGroupCardsType
{
public:
	GD_Type getThisCardType()override {
		return eGDCardType_FourBySingle;
	}

	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, GD_Type& cardType)override
	{
		if (vecCards.size() != 6) {
			return false;
		}
		uint8_t nFind4 = 0;
		uint8_t nCnt = 0;
		uint8_t nSingleCnt = 0;
		for (uint8_t i = 0; i < vecCards.size(); i++) {
			nCnt++;
			auto tValue = DDZ_PARSE_VALUE(vecCards[i]);
			if (i + 1 < vecCards.size() && tValue == DDZ_PARSE_VALUE(vecCards[i + 1])) {
				continue;
			}
			if (nFind4 == 0 && nCnt > 3) {
				nFind4 = tValue;
				nSingleCnt += nCnt - 4;
			}
			else {
				nSingleCnt += nCnt;
			}
			nCnt = 0;
			if (nSingleCnt > 2) {
				return false;
			}
		}

		if (nFind4 && nSingleCnt == 2) {
			cardType = eGDCardType_FourBySingle;
			nWeight = nFind4;
			return true;
		}
		return false;
	}
};

/*
weight : 单张从大到小依次面值 + 最大单张花色
1 << 19 | 2 << 15 | 3 << 11 | 4 << 7 | 5 << 3 | 5
*/
class GDGroupCardsTypeThreeByDouble
	:public IGDGroupCardsType
{
public:
	GD_Type getThisCardType()override {
		return eGDCardType_ThreeByDouble;
	}

	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, GD_Type& cardType)override
	{
		if (vecCards.size() != 5) {
			return false;
		}
		uint8_t nFind3 = 0;
		uint8_t nCnt = 0;
		uint8_t nPairCnt = 0;
		for (uint8_t i = 0; i < vecCards.size(); i++) {
			nCnt++;
			auto tValue = DDZ_PARSE_VALUE(vecCards[i]);
			if (i + 1 < vecCards.size() && tValue == DDZ_PARSE_VALUE(vecCards[i + 1])) {
				continue;
			}
			if (nFind3 == 0 && nCnt > 2) {
				if ((nCnt - 3) % 2) {
					return false;
				}
				nFind3 = tValue;
				nPairCnt += (nCnt - 3) / 2;
			}
			else {
				if (nCnt % 2) {
					return false;
				}
				nPairCnt += nCnt / 2;
			}
			nCnt = 0;
			if (nPairCnt > 1) {
				return false;
			}
		}

		if (nFind3 && nPairCnt == 1) {
			cardType = eGDCardType_ThreeByDouble;
			nWeight = nFind3;
			return true;
		}
		return false;
	}
};

class GDGroupCardsTypeThreeBySingle
	:public IGDGroupCardsType
{
public:
	GD_Type getThisCardType()override {
		return eGDCardType_ThreeBySingle;
	}

	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, GD_Type& cardType)override
	{
		if (vecCards.size() != 4) {
			return false;
		}
		uint8_t nFind3 = 0;
		uint8_t nCnt = 0;
		uint8_t nSingleCnt = 0;
		for (uint8_t i = 0; i < vecCards.size(); i++) {
			nCnt++;
			auto tValue = DDZ_PARSE_VALUE(vecCards[i]);
			if (i + 1 < vecCards.size() && tValue == DDZ_PARSE_VALUE(vecCards[i + 1])) {
				continue;
			}
			if (nFind3 == 0 && nCnt > 2) {
				nFind3 = tValue;
				nSingleCnt += (nCnt - 3);
			}
			else {
				nSingleCnt += nCnt;
			}
			nCnt = 0;
			if (nSingleCnt > 1) {
				return false;
			}
		}

		if (nFind3 && nSingleCnt == 1) {
			cardType = eGDCardType_ThreeBySingle;
			nWeight = nFind3;
			return true;
		}
		return false;
	}
};

class GDGroupCardsTypeThreeCards
	:public IGDGroupCardsType
{
public:
	GD_Type getThisCardType()override {
		return eGDCardType_ThreeCards;
	}

	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, GD_Type& cardType)override
	{
		if (vecCards.size() != 3) {
			return false;
		}
		auto nValue = DDZ_PARSE_VALUE(vecCards[0]);
		uint8_t nCnt = std::count_if(vecCards.begin(), vecCards.end(), [nValue](uint8_t& nCard) {
			return DDZ_PARSE_VALUE(nCard) == nValue;
		});
		if (nCnt == 3) {
			cardType = eGDCardType_ThreeCards;
			nWeight = nValue;
			return true;
		}
		return false;
	}
};

class GDGroupCardsTypeDouble
	:public IGDGroupCardsType
{
public:
	GD_Type getThisCardType()override {
		return eGDCardType_Double;
	}

	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, GD_Type& cardType)override
	{
		if (vecCards.size() != 2) {
			return false;
		}
		auto nValue = DDZ_PARSE_VALUE(vecCards[0]);
		uint8_t nCnt = std::count_if(vecCards.begin(), vecCards.end(), [nValue](uint8_t& nCard) {
			return DDZ_PARSE_VALUE(nCard) == nValue;
		});
		if (nCnt == 2) {
			cardType = eGDCardType_Double;
			nWeight = nValue;
			return true;
		}
		return false;
	}
};

class GDGroupCardsTypeSingle
	:public IGDGroupCardsType
{
public:
	GD_Type getThisCardType()override {
		return eGDCardType_Single;
	}

	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, GD_Type& cardType)override
	{
		if (vecCards.size() != 1) {
			return false;
		}
		auto nValue = DDZ_PARSE_VALUE(vecCards[0]);
		cardType = eGDCardType_Single;
		nWeight = nValue;
		return true;
	}
};

class GDCardTypeChecker
	:public CSingleton<GDCardTypeChecker>
{
public:
	GDCardTypeChecker()
	{
		for (auto& ref : m_vCardTypes) {
			if (ref.second) {
				delete ref.second;
				ref.second = nullptr;
			}
		}
		m_vCardTypes.clear();

		IGDGroupCardsType* p = new GDGroupCardsTypeJokerBomb();
		m_vCardTypes[p->getThisCardType()] = p;

		p = new GDGroupCardsTypeFourBomb();
		m_vCardTypes[p->getThisCardType()] = p;

		p = new GDGroupCardsTypeThreeStraightByDouble();
		m_vCardTypes[p->getThisCardType()] = p;

		p = new GDGroupCardsTypeThreeStraightBySingle();
		m_vCardTypes[p->getThisCardType()] = p;

		p = new GDGroupCardsTypeThreeStraight();
		m_vCardTypes[p->getThisCardType()] = p;

		p = new GDGroupCardsTypeDoubleStraight();
		m_vCardTypes[p->getThisCardType()] = p;

		p = new GDGroupCardsTypeStraight();
		m_vCardTypes[p->getThisCardType()] = p;

		p = new GDGroupCardsTypeFourByDouble();
		m_vCardTypes[p->getThisCardType()] = p;

		p = new GDGroupCardsTypeFourBySingle();
		m_vCardTypes[p->getThisCardType()] = p;

		p = new GDGroupCardsTypeThreeByDouble();
		m_vCardTypes[p->getThisCardType()] = p;

		p = new GDGroupCardsTypeThreeBySingle();
		m_vCardTypes[p->getThisCardType()] = p;

		p = new GDGroupCardsTypeThreeCards();
		m_vCardTypes[p->getThisCardType()] = p;

		p = new GDGroupCardsTypeDouble();
		m_vCardTypes[p->getThisCardType()] = p;

		p = new GDGroupCardsTypeSingle();
		m_vCardTypes[p->getThisCardType()] = p;
	}

public:
	~GDCardTypeChecker()
	{
		for (auto& ref : m_vCardTypes)
		{
			if (ref.second) {
				delete ref.second;
				ref.second = nullptr;
			}
		}
		m_vCardTypes.clear();
	}
public:
	bool checkCardType(std::vector<uint8_t> vecCards, uint32_t& nWeight, GD_Type& cardType, GD_Type nCheckType = eGDCardType_None)
	{
		if (vecCards.size() < 1)
		{
			return false;
		}

		if (eGDCardType_None == nCheckType) {
			for (auto& ref : m_vCardTypes)
			{
				if (ref.second->isThisCardType(vecCards, nWeight, cardType))
				{
					return true;
				}
			}
		}
		else {
			if (m_vCardTypes.count(nCheckType)) {
				auto ref = m_vCardTypes[nCheckType];
				return ref->isThisCardType(vecCards, nWeight, cardType);
			}
		}

		return false;
	}
protected:
	std::map<uint8_t, IGDGroupCardsType*> m_vCardTypes;
};