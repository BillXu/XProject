#pragma once
#include "Singleton.h"
class IThirteenCardType
{
public:
	virtual bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, ThirteenType& cardType) = 0;
};

/*
	重要面值1（4） | 重要面值2（4）| 重要面值3（4）| 重要牌花色类型(4)， 总共32位
	A>K>Q>J>10>9...>2
	黑桃>红桃>梅花>方块
	对子+单张，同花，顺子比最大单张花色
	1,2,3,4,5 是最小的顺子 比A花色
	头道3张牌只有单张，对子，三张
*/

class ThirteenCardType5Tong
	:public IThirteenCardType
{
public:
	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, ThirteenType& cardType)override
	{
		if (vecCards.size() != OTHER_DAO_CARD_COUNT) {
			return false;
		}
		uint8_t nJokerCnt = std::count_if(vecCards.begin(), vecCards.end(), [](uint8_t& nCard) {
			return TT_PARSE_TYPE(nCard) == ePoker_Joker;
		});
		if (nJokerCnt > 4) {
			nWeight = 14;
			cardType = Thirteen_5Tong;
			return true;
		}
		else {
			for (uint8_t i = 0; i < vecCards.size(); i++) {
				auto tType = TT_PARSE_TYPE(vecCards[i]);
				if (tType == ePoker_Joker) {
					continue;
				}
				auto tValue = TT_PARSE_VALUE(vecCards[i]);
				if (i + 1 < vecCards.size() && tValue == TT_PARSE_VALUE(vecCards[i + 1])) {
					continue;
				}
				uint8_t nCnt = std::count_if(vecCards.begin(), vecCards.end(), [tValue](uint8_t& nCard) {
					return TT_PARSE_VALUE(nCard) == tValue;
				});
				if (nCnt + nJokerCnt > 4) {
					nWeight = tValue;
					cardType = Thirteen_5Tong;
					return true;
				}
			}
			return false;
		}
	}
};

/*
	weight : 单张最大面值 + 花色
*/
class ThirteenCardTypeStraightFlush
	:public IThirteenCardType
{
public:
	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, ThirteenType& cardType)override
	{
		if (vecCards.size() != OTHER_DAO_CARD_COUNT) {
			return false;
		}

		//检测同花并确定花色
		uint8_t flushType = -1;
		uint8_t nJokerCnt = 0;
		std::vector<uint8_t> vCards;
		for (auto ref : vecCards) {
			auto tType = TT_PARSE_TYPE(ref);
			if (tType == ePoker_Joker) {
				nJokerCnt++;
				continue;
			}
			vCards.push_back(ref);
			if ((uint8_t)-1 == flushType) {
				flushType = tType;
			}
			else {
				if (flushType == tType) {
					continue;
				}
				return false;
			}
		}
		if ((uint8_t)-1 == flushType) {
			flushType = ePoker_Sword;
		}

		//检测顺子
		if (vCards.size() && TT_PARSE_VALUE(vCards[0]) == 14) {
			if (std::find_if(vCards.begin(), vCards.end(), [](uint8_t& nCard) {
				auto nValue = TT_PARSE_VALUE(nCard);
				return nValue != 14 && TT_PARSE_VALUE(nCard) > OTHER_DAO_CARD_COUNT;
			}) == vCards.end()) {
				for (auto& ref : vCards) {
					if (TT_PARSE_VALUE(ref) == 14) {
						ref = TT_MAKE_CARD(TT_PARSE_TYPE(ref), 1);
					}
				}
				std::sort(vCards.begin(), vCards.end(), [](uint8_t nCard_1, uint8_t nCard_2) {
					return nCard_1 > nCard_2;
				});
			}
			else {
				//TODO NOTHING
			}
		}
		uint8_t needJokerCnt = 0;
		for (uint8_t i = 0; i < vCards.size(); i++) {
			if (i + 1 < vCards.size()) {
				auto tValue_1 = TT_PARSE_VALUE(vCards[i]);
				auto tValue_2 = TT_PARSE_VALUE(vCards[i + 1]);
				if (tValue_1 > tValue_2) {
					uint8_t tDis = tValue_1 - tValue_2 - 1;
					needJokerCnt += tDis;
					if (needJokerCnt > nJokerCnt) {
						return false;
					}
					else {
						for (uint8_t j = 0; j < tDis; j++) {
							IPeerCard::addCardToVecAsc(vCards, TT_MAKE_CARD(flushType, tValue_1 - 1 - j));
						}
					}
				}
				else {
					return false;
				}
			}
		}
		uint8_t nLeftJokerCnt = nJokerCnt - needJokerCnt;
		for (uint8_t i = 0; i < nLeftJokerCnt; i++) {
			if (vCards.size() && TT_PARSE_VALUE(vCards[0]) == 14) {
				auto cValue = TT_PARSE_VALUE(vCards[vCards.size() - 1]);
				if (cValue > 1) {
					vCards.push_back(TT_MAKE_CARD(flushType, cValue - 1));
				}
			}
			else {
				if (vCards.size()) {
					if (TT_PARSE_VALUE(vCards[0]) == OTHER_DAO_CARD_COUNT) {
						IPeerCard::addCardToVecAsc(vCards, TT_MAKE_CARD(flushType, TT_PARSE_VALUE(vCards[vCards.size() - 1]) - 1));
					}
					else {
						IPeerCard::addCardToVecAsc(vCards, TT_MAKE_CARD(flushType, TT_PARSE_VALUE(vCards[0]) + 1));
					}
				}
				else {
					IPeerCard::addCardToVecAsc(vCards, TT_MAKE_CARD(flushType, 14));
				}
			}
		}

		nWeight = TT_PARSE_VALUE(vCards[0]);
		if (nWeight == 14) {
			nWeight = 15;
		}
		else if (nWeight == OTHER_DAO_CARD_COUNT) {
			nWeight = 14;
		}
		nWeight = (nWeight << 3) | flushType;
		cardType = Thirteen_StraightFlush;
		return true;
	}
};

class ThirteenCardTypeTieZhi
	:public IThirteenCardType
{
public:
	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, ThirteenType& cardType)override
	{
		if (vecCards.size() != OTHER_DAO_CARD_COUNT) {
			return false;
		}
		uint8_t nJokerCnt = std::count_if(vecCards.begin(), vecCards.end(), [](uint8_t& nCard) {
			return TT_PARSE_TYPE(nCard) == ePoker_Joker;
		});
		if (nJokerCnt > 3) {
			nWeight = 14;
			cardType = Thirteen_TieZhi;
			return true;
		}
		else {
			for (uint8_t i = 0; i < vecCards.size(); i++) {
				auto tType = TT_PARSE_TYPE(vecCards[i]);
				if (tType == ePoker_Joker) {
					continue;
				}
				auto tValue = TT_PARSE_VALUE(vecCards[i]);
				if (i + 1 < vecCards.size() && tValue == TT_PARSE_VALUE(vecCards[i + 1])) {
					continue;
				}
				uint8_t nCnt = std::count_if(vecCards.begin(), vecCards.end(), [tValue](uint8_t& nCard) {
					return TT_PARSE_VALUE(nCard) == tValue;
				});
				if (nCnt + nJokerCnt > 3) {
					nWeight = tValue;
					cardType = Thirteen_TieZhi;
					return true;
				}
			}
			return false;
		}
	}
};

class ThirteenCardTypeFuLu
	:public IThirteenCardType
{
public:
	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, ThirteenType& cardType)override
	{
		if (vecCards.size() != OTHER_DAO_CARD_COUNT) {
			return false;
		}
		uint8_t nJokerCnt = std::count_if(vecCards.begin(), vecCards.end(), [](uint8_t& nCard) {
			return TT_PARSE_TYPE(nCard) == ePoker_Joker;
		});
		if (nJokerCnt > 1) {
			return false;
		}
		uint8_t useJokerCnt = nJokerCnt;
		uint8_t find3 = 0;
		bool have3Sword = false;
		uint8_t find2 = 0;
		for (uint8_t i = 0; i < vecCards.size(); i++) {
			bool haveSword = false;
			auto tType = TT_PARSE_TYPE(vecCards[i]);
			if (tType == ePoker_Joker) {
				continue;
			}
			auto tValue = TT_PARSE_VALUE(vecCards[i]);
			if (i + 1 < vecCards.size() && tValue == TT_PARSE_VALUE(vecCards[i + 1])) {
				continue;
			}
			uint8_t nCnt = std::count_if(vecCards.begin(), vecCards.end(), [tValue, &haveSword](uint8_t& nCard) {
				if (TT_PARSE_TYPE(nCard) == ePoker_Sword) {
					haveSword = true;
				}
				return TT_PARSE_VALUE(nCard) == tValue;
			});
			if (nCnt + useJokerCnt == 3) {
				if (find3) {
					return false;
				}
				else {
					if (haveSword) {
						have3Sword = true;
					}
					find3 = tValue;
					useJokerCnt = 0;
				}
			}
			else if (nCnt == 2) {
				if (find2) {
					continue;
				}
				else {
					find2 = tValue;
				}
			}
		}
		if (find3 && find2) {
			nWeight = (find3 << 8) | (find2 << 4) | (nJokerCnt ? (have3Sword ? ePoker_Sword : 0) : ePoker_Sword + 1);
			cardType = Thirteen_FuLu;
			return true;
		}
		return false;
	}
};

class ThirteenCardTypeFlush
	:public IThirteenCardType
{
public:
	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, ThirteenType& cardType)override
	{
		if (vecCards.size() != OTHER_DAO_CARD_COUNT) {
			return false;
		}
		uint8_t flushType = -1;
		uint8_t nJokerCnt = 0;
		std::vector<uint8_t> vCards;
		for (auto ref : vecCards) {
			auto tType = TT_PARSE_TYPE(ref);
			if (tType == ePoker_Joker) {
				nJokerCnt++;
				continue;
			}
			vCards.push_back(ref);
			if ((uint8_t)-1 == flushType) {
				flushType = tType;
			}
			else{
				if (flushType == tType) {
					continue;
				}
				return false;
			}
		}

		if (nJokerCnt) {
			for (uint8_t i = 0; i < nJokerCnt; i++) {
				for (uint8_t tValue = 14; tValue > 1; tValue--) {
					auto tCard = TT_MAKE_CARD(flushType, tValue);
					if (std::find(vCards.begin(), vCards.end(), tCard) == vCards.end()) {
						IPeerCard::addCardToVecAsc(vCards, tCard);
					}
				}
			}
		}
		for (auto& ref : vCards) {
			nWeight = (nWeight << 4) | TT_PARSE_VALUE(ref);
		}
		if ((uint8_t)-1 != flushType) {
			nWeight = (nWeight << 3) | flushType;
		}
		cardType = Thirteen_Flush;
		return true;
	}
};

/*
	weight : 最大一张牌面值 + 5张牌从大到小一次的花色
*/
class ThirteenCardTypeStraight
	:public IThirteenCardType
{
public:
	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, ThirteenType& cardType)override
	{
		if (vecCards.size() != OTHER_DAO_CARD_COUNT) {
			return false;
		}
		uint8_t nJokerCnt = 0;
		std::vector<uint8_t> vCards;
		for (auto ref : vecCards) {
			auto tType = TT_PARSE_TYPE(ref);
			if (tType == ePoker_Joker) {
				nJokerCnt++;
				continue;
			}
			vCards.push_back(ref);
		}
		bool is12345 = false;
		if (vCards.size() && TT_PARSE_VALUE(vCards[0]) == 14) {
			if (std::find_if(vCards.begin(), vCards.end(), [](uint8_t& nCard) {
				auto nValue = TT_PARSE_VALUE(nCard);
				return nValue != 14 && nValue > OTHER_DAO_CARD_COUNT;
			}) == vCards.end()) {
				is12345 = true;
				for (auto& ref : vCards) {
					if (TT_PARSE_VALUE(ref) == 14) {
						ref = TT_MAKE_CARD(TT_PARSE_TYPE(ref), 1);
					}
				}
				std::sort(vCards.begin(), vCards.end(), [](uint8_t nCard_1, uint8_t nCard_2) {
					return nCard_1 > nCard_2;
				});
			}
			//else {
			//	//TODO NOTHING
			//}
		}
		uint8_t needJokerCnt = 0;
		for (uint8_t i = 0; i < vCards.size(); i++) {
			if (i + 1 < vCards.size()) {
				auto tValue_1 = TT_PARSE_VALUE(vCards[i]);
				auto tValue_2 = TT_PARSE_VALUE(vCards[i + 1]);
				if (tValue_1 > tValue_2) {
					uint8_t tDis = tValue_1 - tValue_2 - 1;
					needJokerCnt += tDis;
					if (needJokerCnt > nJokerCnt) {
						return false;
					}
					else {
						for (uint8_t j = 0; j < tDis; j++) {
							IPeerCard::addCardToVecAsc(vCards, TT_MAKE_CARD(ePoker_Sword, tValue_1 - 1 - j));
						}
					}
				}
				else {
					return false;
				}
			}
		}
		uint8_t nLeftJokerCnt = nJokerCnt - needJokerCnt;
		for (uint8_t i = 0; i < nLeftJokerCnt; i++) {
			if (vCards.size() && TT_PARSE_VALUE(vCards[0]) == 14) {
				auto cValue = TT_PARSE_VALUE(vCards[vCards.size() - 1]);
				if (cValue > 1) {
					vCards.push_back(TT_MAKE_CARD(ePoker_Sword, cValue - 1));
				}
			}
			else {
				if (vCards.size()) {
					if (TT_PARSE_VALUE(vCards[0]) == OTHER_DAO_CARD_COUNT) {
						IPeerCard::addCardToVecAsc(vCards, TT_MAKE_CARD(ePoker_Sword, TT_PARSE_VALUE(vCards[vCards.size() - 1]) - 1));
					}
					else {
						IPeerCard::addCardToVecAsc(vCards, TT_MAKE_CARD(ePoker_Sword, TT_PARSE_VALUE(vCards[0]) + 1));
					}
				}
				else {
					IPeerCard::addCardToVecAsc(vCards, TT_MAKE_CARD(ePoker_Sword, 14));
				}
			}
		}

		nWeight = TT_PARSE_VALUE(vCards[0]);
		if (nWeight == 14) {
			nWeight = 15;
		}
		else if (nWeight == OTHER_DAO_CARD_COUNT) {
			nWeight = 14;
		}

		if (is12345) {
			for (auto& ref : vCards) {
				if (TT_PARSE_VALUE(ref) == 1) {
					ref = TT_MAKE_CARD(TT_PARSE_TYPE(ref), 14);
				}
			}
			std::sort(vCards.begin(), vCards.end(), [](uint8_t nCard_1, uint8_t nCard_2) {
				return nCard_1 > nCard_2;
			});
		}

		for (auto& ref : vCards) {
			uint8_t nHuaWeight = 0;
			if (std::find(vecCards.begin(), vecCards.end(), ref) == vecCards.end()) {
				nHuaWeight = ePoker_Sword;
			}
			else {
				auto refType = TT_PARSE_TYPE(ref);
				nHuaWeight = (refType == ePoker_Sword ? refType + 1 : refType);
			}
			nWeight = (nWeight << 3) | nHuaWeight;
		}
		cardType = Thirteen_Straight;
		return true;
	}
};

/*
	weight : 3张的牌面值 + 单张面值 + 大单张花色
*/
class ThirteenCardTypeThreeCards
	:public IThirteenCardType
{
public:
	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, ThirteenType& cardType)override
	{
		if (vecCards.size() > OTHER_DAO_CARD_COUNT) {
			return false;
		}
		uint8_t nJokerCnt = 0;
		std::vector<uint8_t> vCards;
		for (auto ref : vecCards) {
			auto tType = TT_PARSE_TYPE(ref);
			if (tType == ePoker_Joker) {
				nJokerCnt++;
				continue;
			}
			vCards.push_back(ref);
		}
		if (nJokerCnt > 3) {
			return false;
		}
		else if (nJokerCnt == 3) {
			nWeight = 14 << ((OTHER_DAO_CARD_COUNT - 3) * 4 + 3);
			uint32_t nWeightOgher = 0;
			uint8_t bigType = 0;
			for (uint8_t i = 0; i < vCards.size(); i++) {
				auto tValue = TT_PARSE_VALUE(vCards[i]);
				nWeightOgher = (nWeightOgher << 4) | tValue;
				if (bigType) {
					continue;
				}
				else {
					bigType = TT_PARSE_TYPE(vCards[i]);
				}
			}
			if (bigType) {
				nWeightOgher = (nWeightOgher << 3) | bigType;
			}
			nWeight = nWeight | nWeightOgher;
			cardType = Thirteen_ThreeCards;
			return true;
		}

		uint8_t find3 = 0;
		for (uint8_t i = 0; i < vCards.size(); i++) {
			auto tValue = TT_PARSE_VALUE(vCards[i]);
			if (i + 1 < vCards.size() && tValue == TT_PARSE_VALUE(vCards[i + 1])) {
				continue;
			}
			uint8_t nCnt = std::count_if(vCards.begin(), vCards.end(), [tValue](uint8_t& nCard) {
				return TT_PARSE_VALUE(nCard) == tValue;
			});
			if (nCnt + nJokerCnt == 3) {
				find3 = tValue;
				break;
			}
		}
		if (find3) {
			nWeight = find3 << ((OTHER_DAO_CARD_COUNT - 3) * 4 + 3);
			uint32_t nWeightOgher = 0;
			uint8_t bigType = -1;
			for (uint8_t i = 0; i < vCards.size(); i++) {
				auto tValue = TT_PARSE_VALUE(vCards[i]);
				if (tValue == find3) {
					continue;
				}
				nWeightOgher = (nWeightOgher << 4) | tValue;
				if ((uint8_t)-1 == bigType) {
					bigType = TT_PARSE_TYPE(vCards[i]);
				}
			}
			if ((uint8_t)-1 != bigType) {
				nWeightOgher = (nWeightOgher << 3) | bigType;
			}
			nWeight = nWeight | nWeightOgher;
			cardType = Thirteen_ThreeCards;
			return true;
		}
		return false;
	}
};

/*
	weight : 大对子牌面 + 小对子牌面 + 单张牌面 + 大单张花色
*/
class ThirteenCardTypeDoubleDouble
	:public IThirteenCardType
{
public:
	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, ThirteenType& cardType)override
	{
		if (vecCards.size() != OTHER_DAO_CARD_COUNT) {
			return false;
		}
		uint8_t nBD = 0;
		uint8_t nSD = 0;
		for (uint8_t i = 0; i < vecCards.size(); i++) {
			if (i + 1 < vecCards.size() && TT_PARSE_VALUE(vecCards[i]) == TT_PARSE_VALUE(vecCards[i + 1])) {
				continue;
			}
			auto tType = TT_PARSE_TYPE(vecCards[i]);
			if (tType == ePoker_Joker) {
				return false;
			}
			auto tValue = TT_PARSE_VALUE(vecCards[i]);
			if (std::count_if(vecCards.begin(), vecCards.end(), [tValue](uint8_t& nCard) {
				return TT_PARSE_VALUE(nCard) == tValue;
			}) == 2) {
				if (nBD) {
					if (nSD) {
						return false;
					}
					else {
						nSD = tValue;
					}
				}
				else {
					nBD = tValue;
				}
			}
		}
		if (nBD && nSD) {
			nWeight = (nBD << 4) | nSD;
			uint8_t bigType = -1;
			for (auto& ref : vecCards) {
				auto refValue = TT_PARSE_VALUE(ref);
				if (refValue == nBD || refValue == nSD) {
					continue;
				}
				nWeight = (nWeight << 4) | refValue;
				if ((uint8_t)-1 == bigType) {
					bigType = TT_PARSE_TYPE(ref);
				}
			}
			if ((uint8_t)-1 != bigType) {
				nWeight = (nWeight << 3) | bigType;
			}
			cardType = Thirteen_DoubleDouble;
			return true;
		}
		return false;
	}
};

/*
	weight : 对子面值(4) + 单张从大到小一次面值(4) + 大单张花色(3)
*/
class ThirteenCardTypeDouble
	:public IThirteenCardType
{
public:
	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, ThirteenType& cardType)override
	{
		if (vecCards.size() > OTHER_DAO_CARD_COUNT) {
			return false;
		}
		uint8_t nJokerCnt = 0;
		std::vector<uint8_t> vCards;
		for (auto ref : vecCards) {
			auto tType = TT_PARSE_TYPE(ref);
			if (tType == ePoker_Joker) {
				nJokerCnt++;
				continue;
			}
			vCards.push_back(ref);
		}
		if (nJokerCnt > 1) {
			return false;
		}

		uint8_t find2 = 0;
		for (uint8_t i = 0; i < vCards.size(); i++) {
			auto tValue = TT_PARSE_VALUE(vCards[i]);
			if (i + 1 < vCards.size() && tValue == TT_PARSE_VALUE(vCards[i + 1])) {
				continue;
			}
			uint8_t nCnt = std::count_if(vCards.begin(), vCards.end(), [tValue](uint8_t& nCard) {
				return tValue == TT_PARSE_VALUE(nCard);
			});
			if (nCnt + nJokerCnt == 2) {
				find2 = tValue;
				break;
			}
		}
		if (find2) {
			nWeight = find2 << ((OTHER_DAO_CARD_COUNT - 2) * 4 + 3);
			uint32_t nOtherWeight = 0;
			uint8_t bigType = -1;
			uint8_t nLMCnt = 0;
			for (auto& ref : vCards) {
				auto refValue = TT_PARSE_VALUE(ref);
				if (refValue == find2) {
					continue;
				}
				nLMCnt++;
				nOtherWeight = nOtherWeight | (refValue << ((OTHER_DAO_CARD_COUNT - 2 - nLMCnt) * 4 + 3));
				if ((uint8_t)-1 == bigType) {
					bigType = TT_PARSE_TYPE(ref);
				}
			}
			if ((uint8_t)-1 != bigType) {
				nOtherWeight = nOtherWeight | bigType;
			}
			nWeight = nWeight | nOtherWeight;
			cardType = Thirteen_Double;
			return true;
		}
		return false;
	}
};

/*
	weight : 单张从大到小依次面值 + 最大单张花色
	1 << 19 | 2 << 15 | 3 << 11 | 4 << 7 | 5 << 3 | 5
*/
class ThirteenCardTypeSingle
	:public IThirteenCardType
{
public:
	bool isThisCardType(std::vector<uint8_t>& vecCards, uint32_t& nWeight, ThirteenType& cardType)override
	{
		if (vecCards.size() > OTHER_DAO_CARD_COUNT) {
			return false;
		}
		uint8_t nLMCnt = 0;
		uint8_t bigType = -1;
		for (uint8_t i = 0; i < vecCards.size(); i++) {
			auto tType = TT_PARSE_TYPE(vecCards[i]);
			if (tType == ePoker_Joker) {
				return false;
			}
			auto tValue = TT_PARSE_VALUE(vecCards[i]);
			nLMCnt++;
			//nWeight = (nWeight << ((OTHER_DAO_CARD_COUNT - nLMCnt) * 4 + 3)) | tValue;
			nWeight = nWeight | (tValue << ((OTHER_DAO_CARD_COUNT - nLMCnt) * 4 + 3));
			if ((uint8_t)-1 ==  bigType) {
				bigType = tType;
			}
		}
		if ((uint8_t)-1 != bigType) {
			nWeight = nWeight | bigType;
		}
		cardType = Thirteen_Single;
		return true;
	}
};

class ThirteenCardTypeChecker
	:public CSingleton<ThirteenCardTypeChecker>
{
public:
	ThirteenCardTypeChecker()
	{
		IThirteenCardType* p = new ThirteenCardType5Tong();
		m_vCardTypes.push_back(p);

		p = new ThirteenCardTypeStraightFlush();
		m_vCardTypes.push_back(p);

		p = new ThirteenCardTypeTieZhi();
		m_vCardTypes.push_back(p);

		p = new ThirteenCardTypeFuLu();
		m_vCardTypes.push_back(p);

		p = new ThirteenCardTypeFlush();
		m_vCardTypes.push_back(p);

		p = new ThirteenCardTypeStraight();
		m_vCardTypes.push_back(p);

		p = new ThirteenCardTypeThreeCards();
		m_vCardTypes.push_back(p);

		p = new ThirteenCardTypeDoubleDouble();
		m_vCardTypes.push_back(p);

		p = new ThirteenCardTypeDouble();
		m_vCardTypes.push_back(p);

		p = new ThirteenCardTypeSingle();
		m_vCardTypes.push_back(p);
	}

public:
	~ThirteenCardTypeChecker()
	{
		for (auto& ref : m_vCardTypes)
		{
			delete ref;
			ref = nullptr;
		}
		m_vCardTypes.clear();
	}
public:
	bool checkCardType(std::vector<uint8_t> vecCards, uint32_t& nWeight, ThirteenType& cardType)
	{
		if (vecCards.size() != HEAD_DAO_CARD_COUNT && vecCards.size() != OTHER_DAO_CARD_COUNT)
		{
			return false;
		}

		for (auto& ref : m_vCardTypes)
		{
			if (ref->isThisCardType(vecCards, nWeight, cardType))
			{
				return true;
			}
		}
		return false;
	}
protected:
	std::vector<IThirteenCardType*> m_vCardTypes;
};