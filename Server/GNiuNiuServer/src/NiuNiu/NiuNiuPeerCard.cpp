#include "NiuNiuPeerCard.h"
#include "log4z.h"
#include <algorithm>
CNiuNiuPeerCard::CNiuNiuPeerCard(){
    m_nAddIdx = 0;
    m_eType = Niu_Max;

	m_isEnableFiveFlower = true;
	m_isEnableBoom = true ;
	m_isEnableFiveSmall = true ;
	m_isEnableShunKan = false;
	m_isEnableCrazy = false ;
	m_nBiggestCardIdx = 0;

	m_vHoldCards.resize(NIUNIU_HOLD_CARD_COUNT);
}

void CNiuNiuPeerCard::setOpts(bool isEnableFiveFlower, bool isEnableBoom, bool isEnableFiveSmall, bool isEnableShunKan, bool isEnableCrazy)
{
	m_isEnableFiveFlower = isEnableFiveFlower;
	m_isEnableBoom = isEnableBoom;
	m_isEnableFiveSmall = isEnableFiveSmall;
	m_isEnableShunKan = isEnableShunKan;
	m_isEnableCrazy = isEnableCrazy;
}

CNiuNiuPeerCard::CardGroup CNiuNiuPeerCard::s_CardGroup[10] = { 
	CardGroup(0,1,2,3,4),
	CardGroup(0,2,1,3,4),
	CardGroup(0,3,1,2,4),
	CardGroup(0,4,1,2,3),

	CardGroup(1,2,0,3,4),
	CardGroup(1,3,0,2,4),
	CardGroup(1,4,0,2,3),

	CardGroup(2,3,0,1,4),
	CardGroup(2,4,0,1,3),

	CardGroup(3,4,0,1,2)
} ;
void CNiuNiuPeerCard::addCompositCardNum( uint8_t nCardCompositNum )
{
	assert( (m_nAddIdx < NIUNIU_HOLD_CARD_COUNT) && "too many cards" ) ;
	if ((m_nAddIdx < NIUNIU_HOLD_CARD_COUNT) == false)
	{
		return;
	}

	m_vHoldCards[m_nAddIdx].RsetCardByCompositeNum(nCardCompositNum) ;
	if ( m_nAddIdx != m_nBiggestCardIdx )
	{
		if ( m_vHoldCards[m_nAddIdx].GetCardFaceNum() > m_vHoldCards[m_nBiggestCardIdx].GetCardFaceNum() )
		{
			m_nBiggestCardIdx = m_nAddIdx ;
		}
		else if ( m_vHoldCards[m_nAddIdx].GetCardFaceNum() == m_vHoldCards[m_nBiggestCardIdx].GetCardFaceNum() )
		{
			if ( m_vHoldCards[m_nAddIdx].GetType() > m_vHoldCards[m_nBiggestCardIdx].GetType() )
			{
				m_nBiggestCardIdx = m_nAddIdx ;
			}
		}
	}
	++m_nAddIdx ;
}
#ifndef SERVER
CNiuNiuPeerCard::CardGroup CNiuNiuPeerCard::getCardGroup(){
    if ( ! isCaculated() )
    {
        caculateCards();
    }
    if(m_nGroupIdx<10){
        return CNiuNiuPeerCard::s_CardGroup[m_nGroupIdx];
    }
    return CNiuNiuPeerCard::s_CardGroup[0];
}
#endif
const char*  CNiuNiuPeerCard::getNameString() 
{
	if ( ! isCaculated() )
	{
		caculateCards();
	}
#ifndef SERVER
    std::string str = "niuniu_meiniu";
    switch (m_eType) {
        case Niu_None:
            break;
        case Niu_Single:
            str = StringUtils::format("niuniu_niu_%d",m_nPoint);
            break;
        case Niu_Niu:
            str = "niuniu_niuniu";
            break;
        case Niu_FiveFlower:
            str = "niuniu_wuhuaniu";
            break;
        case Niu_Boom:
            str = "niuniu_zhadan";
            break;
        case Niu_FiveSmall:
            str = "niuniu_wuxiaoniu";
            break;
        default:
            str = "niuniu_meiniu";
            break;
    }
    return Language::getInstance()->get(str.c_str());
#endif
	return "niu niu" ;
}

uint32_t CNiuNiuPeerCard::getWeight() 
{
	if ( ! isCaculated() )
	{
		caculateCards();
	}
	return m_nWeight ;
}

void CNiuNiuPeerCard::reset() 
{
	m_nAddIdx = 0 ;
	m_nBiggestCardIdx = 0 ;
	m_eType = Niu_Max ;
	m_nPoint = 0 ;
	m_nGroupIdx = 10 ;
	m_nWeight = 0 ;
	
}

CNiuNiuPeerCard::NiuNiuType CNiuNiuPeerCard::getType()
{
	if ( ! isCaculated() )
	{
		caculateCards();
	}
	return m_eType ;
}

void CNiuNiuPeerCard::toJson(Json::Value& js)
{
	auto nCnt = getHoldCardCnt();
	for (uint8_t nIdx = 0; nIdx < nCnt; ++nIdx)
	{
		auto nCard = getCardByIdx(nIdx);
		if (nCard == 0)
		{
			LOGFMTE("error invalid card idx = %u ", nIdx);
			break;
		}
		js[js.size()] = nCard;
	}
}

uint8_t CNiuNiuPeerCard::getPoint()
{
	if ( ! isCaculated() )
	{
		caculateCards();
	}
	return m_nPoint ;
}

bool CNiuNiuPeerCard::isCaculated()
{
	return Niu_Max != m_eType ;
}

void CNiuNiuPeerCard::caculateCards()
{
	caculateNiuNiuCards(m_eType, m_nPoint, m_nWeight,m_nGroupIdx, m_vHoldCards);
}

void CNiuNiuPeerCard::caculateCards(NiuNiuType& type, uint8_t& nPoint, uint32_t& nWeight, uint8_t& nGroupIdx, std::vector<CCard>& vHoldCards )
{
	//assert(m_nAddIdx == ( NIUNIU_HOLD_CARD_COUNT ) && "cards not enough" );
	type = Niu_None ;
	nPoint = 0 ;
	if (checkTongHuaShun(vHoldCards))
	{
		type = Niu_TongHuaShun;
		nPoint = 10;
	}
	else if (checkFiveSmall(vHoldCards))
	{
		type = Niu_FiveSmall;
		nPoint = 10;
	}
	else if (checkBoom(vHoldCards))
	{
		type = Niu_Boom;
		nPoint = 10;
	}
    else if ( checkFiveFlower(vHoldCards))
    {
		type = Niu_FiveFlower;
		nPoint = 10 ;
    }
	else if (checkHuLu(vHoldCards))
	{
		type = Niu_Hulu;
	}
	else if (checkTongHuaNiu(vHoldCards))
	{
		type = Niu_TongHuaNiu;
	}
	else if (checkShunZiNiu(vHoldCards))
	{
		type = Niu_ShunZiNiu;
	}
	else
	{
		for ( uint8_t nIdx = 0 ; nIdx < 10 ; ++nIdx )
		{
			CardGroup& ref = s_CardGroup[nIdx] ; 
			if ( checkNiu(vHoldCards,ref) )
			{
				type = Niu_Single ;
                uint8_t t1 = vHoldCards[ref.nTwoIdx[0]].GetCardFaceNum()>10?10: vHoldCards[ref.nTwoIdx[0]].GetCardFaceNum();
                uint8_t t2 = vHoldCards[ref.nTwoIdx[1]].GetCardFaceNum()>10?10: vHoldCards[ref.nTwoIdx[1]].GetCardFaceNum();
                auto nTmpPoint = (t1 + t2) % 10 ;
				if (nTmpPoint == 0 )
				{
					type = Niu_Niu ;
					nPoint = 10 ;
					nGroupIdx = nIdx;
					break;
				}

				if ( nTmpPoint > nPoint )
				{
					nPoint = nTmpPoint;
					nGroupIdx = nIdx;
				}
			}
		} 
	}

	nWeight = 0 ;
	uint8_t nType = type;
	uint8_t nBigFaceNum = 0 ;
	uint8_t nCardType = 0;
	if (isHaveJoker() == false)  // when same type , not have joker , should big than that have joker ;
	{
		nBigFaceNum = vHoldCards[m_nBiggestCardIdx].GetCardFaceNum();
		nCardType = vHoldCards[m_nBiggestCardIdx].GetType();
	}
	else
	{
		// get joker type ;
		for (auto& ref : m_vHoldCards)
		{
			if (CCard::eCard_BigJoker == ref.GetType() || CCard::eCard_Joker == ref.GetType())
			{
				nCardType = ref.GetType();
				break;
			}
		}
	}
	nWeight = (nType << 24) | ( nPoint << 16 ) | (nBigFaceNum << 8) | nCardType  ;
}

bool CNiuNiuPeerCard::checkNiu(std::vector<CCard>& vHoldCards,CardGroup& ref )
{
	uint8_t nTotalPoint = 0 ;
	for (int i = 0; i < 3; i++)
	{
        nTotalPoint += (vHoldCards[ref.nThreeIdx[i]].GetCardFaceNum()>10?10: vHoldCards[ref.nThreeIdx[i]].GetCardFaceNum());
	}

	if ((nTotalPoint % 10) == 0)
	{
		return true;
	}
	
	if ( false == m_isEnableShunKan )
	{
		return false;
	}

	// check shun kan Niu
	std::vector<uint8_t> vec;
	for (int i = 0; i < 3; i++)
	{
		vec.push_back(vHoldCards[ref.nThreeIdx[i]].GetCardFaceNum());
	}
	
	std::sort(vec.begin(),vec.end());
	// check same 
	if ( vec.front() == vec.back() )
	{
		return true;
	}

	// check seq ;
	if (vec[1] == (vec[0] + 1u) && vec[2] == (vec[0] + 2u))
	{
		return true;
	}
	return false;
}

bool CNiuNiuPeerCard::checkFiveSmall(std::vector<CCard>& vHoldCards)
{
	if ( m_isEnableFiveSmall == false)
	{
		return false;
	}

	uint8_t nTotalPoint = 0 ;
	for ( CCard& nCard : vHoldCards)
	{
		if ( nCard.GetCardFaceNum() >= 5 )
		{
			return false ;
		}
		nTotalPoint += nCard.GetCardFaceNum() ;
	}

	return nTotalPoint < 10 ;
}

bool CNiuNiuPeerCard::checkFiveFlower(std::vector<CCard>& vHoldCards)
{
	if ( m_isEnableFiveFlower == false)
	{
		return false;
	}

	for ( CCard& nCard : vHoldCards)
	{
		if ( nCard.GetCardFaceNum() <= 10 )
		{
			return false ;
		}
	}
	return true ;
}

bool CNiuNiuPeerCard::checkBoom(std::vector<CCard>& vHoldCards)
{
	if ( m_isEnableBoom == false)
	{
		return false;
	}

	uint8_t nCard1 = 0, nCard2 = 0 ;
    uint8_t nCard1Count = 0, nCard2Count = 0 ;
	for ( CCard& nCard : vHoldCards)
	{
		if ( nCard1 == 0 )
		{
			nCard1 = nCard.GetCardFaceNum() ;
            nCard1Count++;
			continue;
		}

		if ( nCard1 == nCard.GetCardFaceNum()  && nCard2Count<=1)
		{
            nCard1Count++;
			continue;
		}

		if ( nCard2 == 0 )
		{
			nCard2 = nCard.GetCardFaceNum() ;
            nCard2Count++;
			continue;
		}

		if ( nCard2 == nCard.GetCardFaceNum() && nCard1Count<=1)
		{
            nCard2Count++;
			continue;
		}
		return false ;
	}

	return true ;
}

bool CNiuNiuPeerCard::checkTongHuaShun(std::vector<CCard>& vHoldCards)
{
	if (m_isEnableCrazy == false)
	{
		return false;
	}

	if ( checkTongHuaNiu(vHoldCards) == false)
	{
		return false;
	}

	std::vector<uint8_t> vVec;
	for (auto& ref : vHoldCards)
	{
		vVec.push_back(ref.GetCardCompositeNum());
	}

	std::sort(vVec.begin(), vVec.end());
	for (uint8_t nIdx = 0; (nIdx + 1u) < vVec.size(); ++nIdx)
	{
		if (vVec[nIdx] + 1 != vVec[nIdx + 1])
		{
			return false;
		}
	}
	return true;
}

bool CNiuNiuPeerCard::checkHuLu(std::vector<CCard>& vHoldCards)
{
	if (m_isEnableCrazy == false)
	{
		return false;
	}

	std::vector<uint8_t> vVec;
	for (auto& ref : vHoldCards)
	{
		vVec.push_back(ref.GetCardFaceNum());
	}

	std::sort(vVec.begin(), vVec.end());
	return (vVec[0] == vVec[2] && vVec[3] == vVec[4]) || (vVec[0] == vVec[1] && vVec[2] == vVec[4]);
}

bool CNiuNiuPeerCard::checkTongHuaNiu(std::vector<CCard>& vHoldCards)
{
	if (m_isEnableCrazy == false)
	{
		return false;
	}

	uint8_t nCardType = CCard::eCard_Max;
	for (auto& ref : vHoldCards)
	{
		if (nCardType == CCard::eCard_Max)
		{
			nCardType = ref.GetType();
			continue;
		 }
		
		if (nCardType != ref.GetType())
		{
			return false;
		 }
	}
	return true;
}

bool CNiuNiuPeerCard::checkShunZiNiu(std::vector<CCard>& vHoldCards)
{
	if ( m_isEnableCrazy == false )
	{
		return false;
	}
	std::vector<uint8_t> vVec;
	for (auto& ref : vHoldCards)
	{
		vVec.push_back(ref.GetCardFaceNum());
	}

	std::sort(vVec.begin(), vVec.end());

	for (uint8_t nIdx = 0; (nIdx + 1u) < vVec.size(); ++nIdx)
	{
		if (vVec[nIdx] + 1 != vVec[nIdx + 1])
		{
			return false;
		}
	}
	return true;
}

bool CNiuNiuPeerCard::isHaveJoker()
{
	for (auto& ref : m_vHoldCards )
	{
		if (CCard::eCard_BigJoker == ref.GetType() || CCard::eCard_Joker == ref.GetType())
		{
			return true;
		}
	}
	return false;
}

void CNiuNiuPeerCard::caculateNiuNiuCards(NiuNiuType& type, uint8_t& nPoint, uint32_t& nWeight, uint8_t& nGroupIdx, std::vector<CCard> vHoldCards)
{
	// reset value ;
	type = Niu_None;
	nPoint = 0;
	nWeight = 0;
	// check joker 
	uint8_t nJokerIdx = 100;
	for (uint8_t nIdx = 0; nIdx < vHoldCards.size(); ++nIdx)
	{
		if ( CCard::eCard_BigJoker == vHoldCards[nIdx].GetType() || CCard::eCard_Joker == vHoldCards[nIdx].GetType() )
		{
			nJokerIdx = nIdx;
			break;
		}
	}

	if ( nJokerIdx == 100 )  // current do not have joker 
	{
		caculateCards(type,nPoint,nWeight, nGroupIdx,vHoldCards);
		return;
	}


	NiuNiuType eTmpType = Niu_None;
	uint8_t nTmpPoint = 0;
	uint32_t nTmpWeight = 0;
	uint8_t nTmpGroupIdx = 0;
	for (uint8_t nHuaSe = CCard::eCard_None; nHuaSe < CCard::eCard_NoJoker; ++nHuaSe)
	{
		for (uint8_t nFaceValue = 1; nFaceValue <= 13; ++nFaceValue)
		{
			vHoldCards[nJokerIdx].SetCard((CCard::eCardType)nHuaSe, nFaceValue);
			caculateNiuNiuCards(eTmpType, nTmpPoint, nTmpWeight, nTmpGroupIdx,vHoldCards);
			if (nTmpWeight > nWeight)
			{
				type = eTmpType;
				nPoint = nTmpPoint;
				nWeight = nTmpWeight;
				nGroupIdx = nTmpGroupIdx;
			}
		}

		if ( m_isEnableCrazy == false ) // if not enable crasy niuniu , we donot care hua se , so need not check other hua se ;
		{
			break;
		}
	}
}