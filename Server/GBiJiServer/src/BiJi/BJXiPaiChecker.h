#pragma once
#include "BJDefine.h"
#include "BJPlayerCard.h"
#include <algorithm>
#include <Singleton.h>

class IXiPaiType
{
public:
	virtual bool isThisXiPaiType(BJPlayerCard* pPlayerCard, eXiPaiType& eType) = 0;
};

class IXiPaiAllBlack
	:public IXiPaiType
{
public:
	bool isThisXiPaiType( BJPlayerCard* pPlayerCard, eXiPaiType& eType )override
	{
		std::vector<uint8_t> vHoldCards;
		pPlayerCard->getHoldCards(vHoldCards);
	 
		for (auto& ref : vHoldCards)
		{
			auto nType = BJ_PARSE_TYPE(ref);
			if ( ePoker_Club != nType && ePoker_Sword != nType && ePoker_Joker != nType  )
			{
				return false;
			}
		}

		eType = eXiPai_AllBlack;
		return true;
	}
};

class IXiPaiAllRed
	:public IXiPaiType
{
public:
	bool isThisXiPaiType(BJPlayerCard* pPlayerCard, eXiPaiType& eType)override
	{
		std::vector<uint8_t> vHoldCards;
		pPlayerCard->getHoldCards(vHoldCards);

		for (auto& ref : vHoldCards)
		{
			auto nType = BJ_PARSE_TYPE(ref);
			if (ePoker_Diamond != nType && ePoker_Heart != nType && ePoker_Joker != nType )
			{
				return false;
			}
		}

		eType = eXiPai_AllRed;
		return true;
	}
};

class IXiPaiQuanShun
	:public IXiPaiType
{
public:
	bool isThisXiPaiType(BJPlayerCard* pPlayerCard, eXiPaiType& eType)override
	{
		std::vector<uint8_t> vHoldCardsValue , vHoldUnSort;
		pPlayerCard->getHoldCards(vHoldUnSort);

		uint8_t nJokerCnt = 0;
		for (auto& ref : vHoldUnSort)
		{
			if (ePoker_Joker == BJ_PARSE_TYPE(ref))  // ignore joker, joker can be any card 
			{
				++nJokerCnt;
				continue;
			}

			auto nValue = BJ_PARSE_VALUE(ref);
			if (1 == nValue)
			{
				nValue = 14;
			}
			vHoldCardsValue.push_back(nValue);
		}

		auto pCheckHoldCardShun = []( std::vector<uint8_t> vHoldCards, uint8_t nJokerCnt )
		{
			std::sort(vHoldCards.begin(), vHoldCards.end());
			for (uint8_t nIdx = 0; (nIdx + 1u) < vHoldCards.size(); ++nIdx)
			{
				auto nElapNeedJokeCnt = vHoldCards[nIdx + 1] - (vHoldCards[nIdx] + 1);  // need how many joker can make all sequene
				if (nElapNeedJokeCnt > nJokerCnt)
				{
					return false;
				}
				nJokerCnt -= nElapNeedJokeCnt;
			}
			return true;
		};

		if (pCheckHoldCardShun(vHoldCardsValue,nJokerCnt) )
		{
			eType = eXiPai_QuanShun;
			return true;
		}

		// maybe last A , can be 1 ;
		auto iter = std::find(vHoldCardsValue.begin(),vHoldCardsValue.end(),14 );
		if ( iter == vHoldCardsValue.end() )
		{
			return false;
		}
		else
		{
			*iter = 1;
		}

		if (pCheckHoldCardShun(vHoldCardsValue, nJokerCnt))
		{
			eType = eXiPai_QuanShun;
			return true;
		}

		return false;
	}
};

class IXiPaiShuangShunQing
	:public IXiPaiType
{
public:
	bool isThisXiPaiType(BJPlayerCard* pPlayerCard, eXiPaiType& eType)override
	{
		uint8_t nTongShunCnt = 0;
		for (uint8_t nIdx = 0; nIdx < 3; ++nIdx)
		{
			auto stGroup = pPlayerCard->getGroupByIdx(nIdx);
			if ( CardType_SameColorSequence == stGroup.getType())
			{
				++nTongShunCnt;
			}
		}

		if ( 2 != nTongShunCnt)
		{
			return false;
		}
		eType = eXiPai_ShuangShunQing;
		return true;
	}
};

class IXiPaiShuangSanTiao
	:public IXiPaiType
{
public:
	bool isThisXiPaiType(BJPlayerCard* pPlayerCard, eXiPaiType& eType)override
	{
		uint8_t n3TiaoCnt = 0;
		for (uint8_t nIdx = 0; nIdx < 3; ++nIdx)
		{
			auto stGroup = pPlayerCard->getGroupByIdx(nIdx);
			if (CardType_Bomb == stGroup.getType())
			{
				++n3TiaoCnt;
			}
		}

		if ( 2 != n3TiaoCnt )
		{
			return false;
		}
		eType = eXiPai_ShuangSanTiao;
		return true;
	}
};

class IXiPaiSiZhang
	:public IXiPaiType
{
public:
	bool isThisXiPaiType(BJPlayerCard* pPlayerCard, eXiPaiType& eType)override
	{
		if ( get4ZhangCnt(pPlayerCard) == 1)
		{
			eType = eXiPai_SiZhang;
			return true;
		}
		return false;
	}

	uint8_t get4ZhangCnt( BJPlayerCard* pPlayerCard )
	{
		std::vector<uint16_t> vSanTiaoCards;
		for (uint8_t nIdx = 0; nIdx < 3; ++nIdx)
		{
			auto stGroup = pPlayerCard->getGroupByIdx(nIdx);
			if (CardType_Bomb == stGroup.getType())
			{
				vSanTiaoCards.push_back(stGroup.getCardByIdx(0));
			}
		}

		if (vSanTiaoCards.empty())
		{
			return 0;
		}

		// check 4 zhang 
		std::vector<uint8_t> vHoldCards;
		pPlayerCard->getHoldCards(vHoldCards);

		// hold card conver to face value
		for (auto& ref : vHoldCards)
		{
			if ( ePoker_Joker == BJ_PARSE_TYPE(ref) )  // ignore joker , joker can not be 4 zhang ;
			{
				continue;
			}
			ref = BJ_PARSE_VALUE(ref);
			if (ref == 1)
			{
				ref = 14;
			}
		}

		// check tiao 4 zhang 
		uint8_t n4ZhangCnt = 0;
		for (auto& nTiao : vSanTiaoCards)
		{
			if (std::count(vHoldCards.begin(), vHoldCards.end(), BJ_PARSE_VALUE(nTiao) ) >= 4)
			{
				++n4ZhangCnt;
			}
		}

		return n4ZhangCnt;
	}
};

class IXiPaiShuangSiZhang
	:public IXiPaiSiZhang
{
public:
	bool isThisXiPaiType(BJPlayerCard* pPlayerCard, eXiPaiType& eType)override
	{
		if (get4ZhangCnt(pPlayerCard) == 2 )
		{
			eType = eXiPai_ShuangSiZhang;
			return true;
		}
		return false;
	}
};

class IXiPaiQuanShunQing
	:public IXiPaiType
{
public:
	bool isThisXiPaiType(BJPlayerCard* pPlayerCard, eXiPaiType& eType)override
	{
		for (uint8_t nIdx = 0; nIdx < 3; ++nIdx)
		{
			auto stGroup = pPlayerCard->getGroupByIdx(nIdx);
			if (CardType_SameColorSequence != stGroup.getType())
			{
				return false;
			}
		}
		eType = eXiPai_QuanShunQing;
		return true;
	}
};

class IXiPaiQuanSanTiao
	:public IXiPaiType
{
public:
	bool isThisXiPaiType(BJPlayerCard* pPlayerCard, eXiPaiType& eType)override
	{
		for (uint8_t nIdx = 0; nIdx < 3; ++nIdx)
		{
			auto stGroup = pPlayerCard->getGroupByIdx(nIdx);
			if ( CardType_Bomb != stGroup.getType() )
			{
				return false;
			}
		}
		eType = eXiPai_QuanSanTiao;
		return true;
	}
};

// opts 
class IXiPaiSanQing
	:public IXiPaiType
{
public:
	bool isThisXiPaiType(BJPlayerCard* pPlayerCard, eXiPaiType& eType)override
	{
		for (uint8_t nIdx = 0; nIdx < 3; ++nIdx)
		{
			auto stGroup = pPlayerCard->getGroupByIdx(nIdx);
			if ( CardType_SameColor != stGroup.getType())
			{
				return false;
			}
		}
		eType = eXiPai_SanQing;
		return true;
	}
};

// opts 
class IXiPaiShunQingDaTou
	:public IXiPaiType
{
public:
	bool isThisXiPaiType(BJPlayerCard* pPlayerCard, eXiPaiType& eType)override
	{
		auto stGroup = pPlayerCard->getGroupByIdx(0);
		if ( CardType_SameColorSequence != stGroup.getType())
		{
			return false;
		}

		eType = eXiPai_ShunQingDaTou;
		return true;
	}
};

class BJXiPaiChecker
	:public CSingleton<BJXiPaiChecker>
{
public:
	BJXiPaiChecker()
	{
		IXiPaiType* p = new IXiPaiQuanSanTiao();
		m_vXiPaiTypes.push_back(p);

		p = new IXiPaiQuanShunQing();
		m_vXiPaiTypes.push_back(p);

		p = new IXiPaiShuangSiZhang();
		m_vXiPaiTypes.push_back(p);

		p = new IXiPaiSiZhang();
		m_vXiPaiTypes.push_back(p);

		p = new IXiPaiShuangSanTiao();
		m_vXiPaiTypes.push_back(p);

		p = new IXiPaiShuangShunQing();
		m_vXiPaiTypes.push_back(p);

		p = new IXiPaiQuanShun();
		m_vXiPaiTypes.push_back(p);

		p = new IXiPaiAllBlack();
		m_vXiPaiTypes.push_back(p);

		p = new IXiPaiAllRed();
		m_vXiPaiTypes.push_back(p);
	}

public:
	~BJXiPaiChecker()
	{
		for (auto& ref : m_vXiPaiTypes)
		{
			delete ref;
			ref = nullptr;
		}
		m_vXiPaiTypes.clear();
	}
public:
	bool checkXiPai( BJPlayerCard* pPlayerCard, eXiPaiType& eType, bool isEnableSanQing , bool isEnableShunQingDaTou )
	{
		for (auto& ref : m_vXiPaiTypes)
		{
			if (ref->isThisXiPaiType(pPlayerCard,eType))
			{
				return true;
			}
		}

		if (isEnableSanQing)
		{
			IXiPaiSanQing tTemp;
			if ( tTemp.isThisXiPaiType(pPlayerCard, eType))
			{
				return true;
			}
		}

		if (isEnableShunQingDaTou)
		{
			IXiPaiShunQingDaTou tTemp;
			if (tTemp.isThisXiPaiType(pPlayerCard, eType))
			{
				return true;
			}
		}
		return false;
	}
protected:
	std::vector<IXiPaiType*> m_vXiPaiTypes;
};