#include "MJPlayerCard.h"
#include "MJCard.h"
#include "log4z.h"
// mj player card ;
void MJPlayerCard::reset()
{
	for (auto& vC : m_vCards)
	{
		vC.clear();
	}
	m_vChuedCard.clear();
	m_vPenged.clear();
	m_vDirectGanged.clear();
	m_vBuGang.clear();
	m_vEated.clear();
	m_vAnGanged.clear();
	m_nNesetFetchedCard = 0 ;
	m_nJIang = 0;
	m_nDanDiao = 0;
	m_vLouPenged.clear();
}

void MJPlayerCard::onDoHu(uint16_t nInvokerIdx, uint8_t nHuCard, bool isInvokerHaveGangFlag)
{
	m_nHuPaiInvokerIdx = nInvokerIdx;
	onMoCard(nHuCard);
	if (!isHoldCardCanHu())
	{
		LOGFMTE( "you can not hu , why do hu ?" );
	}
	return;
}

void MJPlayerCard::addDistributeCard(uint8_t nCardNum)
{
	addHoldCard(nCardNum);
}

bool MJPlayerCard::onGangCardBeRobot(uint8_t nCardNum)
{
	// must test vCard is orig vector ;
	auto eType = card_Type(nCardNum);
	if (eType >= eCT_Max)
	{
		LOGFMTE("parse card type error ,gang be robot have this card = %u", nCardNum);
		return false;
	}
	auto& vCard = m_vCards[eType];
	auto iter = std::find(vCard.begin(),vCard.end(),nCardNum);
	if (iter != vCard.end())
	{
		vCard.erase(iter);
		return true;
	}
	LOGFMTE("robot the gang card but player do not have card = %u",nCardNum);
	return false;
}

bool MJPlayerCard::onCardBeGangPengEat(uint8_t nCardNum)
{
	auto eType = card_Type(nCardNum);
	auto& vCard = m_vChuedCard;
	auto iter = std::find(vCard.begin(), vCard.end(), nCardNum);
	if (iter != vCard.end())
	{
		vCard.erase(iter);
		return true;
	}
	LOGFMTE("gang eat peng card but player do not have card = %u", nCardNum);
	return false;
}

bool MJPlayerCard::isHaveCard(uint8_t nCardNum)
{
	auto eType = card_Type(nCardNum);
	if (eType >= eCT_Max)
	{
		LOGFMTE("parse card type error so do not have this card = %u",nCardNum);
		return false;
	}

	auto& vCard = m_vCards[eType];
	auto iter = std::find(vCard.begin(), vCard.end(), nCardNum);
	return iter != vCard.end();
}

bool MJPlayerCard::canMingGangWithCard(uint8_t nCard)
{
	auto eType = card_Type(nCard);
	if (eType >= eCT_Max)
	{
		LOGFMTE("canMingGangWithCard parse card type error so do not have this card = %u", nCard);
		return false;
	}
	auto& vCard = m_vCards[eType];
	auto nCnt = std::count(vCard.begin(), vCard.end(), nCard);
	return nCnt >= 3;
}

bool MJPlayerCard::canPengWithCard(uint8_t nCard)
{
	auto eType = card_Type(nCard);
	if (eType >= eCT_Max)
	{
		LOGFMTE("canPengWithCard parse card type error so do not have this card = %u", nCard);
		return false;
	}

	// is in lou peng vec 
	auto nLouCnt = std::count(m_vLouPenged.begin(), m_vLouPenged.end(), nCard);
	if ( nLouCnt )
	{
		LOGFMTD("this card is in lou peng vec , so can not peng = %u",nCard );
		return false;
	}

	auto& vCard = m_vCards[eType];
	auto nCnt = std::count(vCard.begin(), vCard.end(), nCard);
	return nCnt >= 2;
}

bool MJPlayerCard::canEatCard(uint8_t nCard )
{
	auto eType = card_Type(nCard);
	if (eType >= eCT_Max)
	{
		LOGFMTE("canEatCard parse card type error so do not have this card = %u", nCard);
		return false;
	}

	if (eType != eCT_Tiao && eCT_Tong != eType && eCT_Wan != eType)
	{
		LOGFMTD("only wan , tiao , tong can do eat act");
		return false;
	}

	// ABX ;
	auto nCardValue = card_Value(nCard);
	auto nWithA = nCard - 2;
	auto withB = nCard - 1;
	if ( nCardValue >= 3 && isHaveCard(nWithA) && isHaveCard(withB))
	{
		return true;
	}
	// AXB ;
	nWithA = nCard - 1;
	withB = nCard + 1;
	if ( nCardValue >= 2 &&  isHaveCard(nWithA) && isHaveCard(withB))
	{
		return true;
	}
	// XAB
	nWithA = nCard + 1;
	withB = nCard + 2;
	if ( nCardValue <= 7 &&  isHaveCard(nWithA) && isHaveCard(withB))
	{
		return true;
	}
	return false;
}

bool MJPlayerCard::canHuWitCard(uint8_t nCard)
{
	auto eType = card_Type(nCard);
	if (eType >= eCT_Max)
	{
		LOGFMTE("parse card type error ,canHuWitCard have this card = %u", nCard);
		return false;
	}

	addHoldCard(nCard);
	bool bSelfHu = isHoldCardCanHu();
	removeHoldCard(nCard);
	//debugCardInfo();
	return bSelfHu;
}

bool MJPlayerCard::canAnGangWithCard(uint8_t nCard)
{
	auto eType = card_Type(nCard);
	if (eType >= eCT_Max)
	{
		LOGFMTE("canAnGangWithCard parse card type error so do not have this card = %u", nCard);
		return false;
	}
	auto& vCard = m_vCards[eType];
	auto nCnt = std::count(vCard.begin(), vCard.end(), nCard);
	return nCnt >= 4;
}

bool MJPlayerCard::canBuGangWithCard(uint8_t nCard)
{
	auto eType = card_Type(nCard);
	if (eType >= eCT_Max)
	{
		LOGFMTE("canAnGangWithCard parse card type error so do not have this card = %u", nCard);
		return false;
	}

	if ( isHaveCard(nCard) == false)
	{
		return false;
	}

	return m_vPenged.end() != std::find_if(m_vPenged.begin(), m_vPenged.end(), [nCard](VEC_INVOKE_ACT_INFO::value_type& ref) { return ref.nCard == nCard; });
}

bool MJPlayerCard::isTingPai( uint8_t& nTingCardType )
{
	uint8_t nJiang = 0;
	if ( is7PairTing(nJiang) )
	{
		nTingCardType = card_Type(nJiang);
		return true;
	}

	return isNormalTing();
}

bool MJPlayerCard::getHoldCardThatCanAnGang(VEC_CARD& vGangCards)
{
	for (auto& vCard : m_vCards)
	{
		if ( vCard.size() < 4 )
		{
			continue;
		}

		for (uint8_t nIdx = 0; (uint8_t)(nIdx + 3) < vCard.size();)
		{
			if (vCard[nIdx] == vCard[nIdx + 3])
			{
				vGangCards.push_back(vCard[nIdx]);
				nIdx += 4;
			}
			else
			{
				++nIdx;
			}
		}
	}
	return !vGangCards.empty();
}

bool MJPlayerCard::getHoldCardThatCanBuGang(VEC_CARD& vGangCards)
{
	for (auto& ref : m_vPenged)
	{
		if (isHaveCard(ref.nCard))
		{
			vGangCards.push_back(ref.nCard);
		}
	}

	return !vGangCards.empty();
}

bool MJPlayerCard::isHoldCardCanHu()
{
	if (canHoldCard7PairHu())
	{
		return true;
	}

	uint8_t nJiang = 0;
	return isHoldCardCanHuNormal(nJiang);
}

void MJPlayerCard::onMoCard(uint8_t nMoCard)
{
	auto eType = card_Type(nMoCard);
	if (eType >= eCT_Max)
	{
		LOGFMTE("onMoCard parse card type error so do not have this card = %u", nMoCard);
		return ;
	}
	addHoldCard(nMoCard);
	m_nNesetFetchedCard = nMoCard;
	debugCardInfo();
}

bool MJPlayerCard::onPeng(uint8_t nCard, uint16_t nInvokerIdx )
{
	auto eType = card_Type(nCard);
	if (eType >= eCT_Max)
	{
		LOGFMTE("onPeng parse card type error so do not have this card = %u", nCard);
		return false;
	}
	
	auto& vCard = m_vCards[eType];
	auto nEraseCnt = 2;
	while (nEraseCnt-- > 0)
	{
		auto iter = std::find(vCard.begin(), vCard.end(), nCard);
		vCard.erase(iter);
	}

	// add peng info 
	VEC_INVOKE_ACT_INFO::value_type tPeng;
	tPeng.nCard = nCard;
	tPeng.nInvokerIdx = nInvokerIdx;
	m_vPenged.push_back(tPeng);
	//debugCardInfo();
	return true;
}

bool MJPlayerCard::onDirectGang(uint8_t nCard, uint8_t nGangGetCard, uint16_t nInvokerIdx )
{
	auto eType = card_Type(nCard);
	if (eType >= eCT_Max)
	{
		LOGFMTE("onMingGang parse card type error so do not have this card = %u", nCard);
		return false;
	}

	auto& vCard = m_vCards[eType];
	auto nEraseCnt = 3;
	while (nEraseCnt-- > 0)
	{
		auto iter = std::find(vCard.begin(), vCard.end(), nCard);
		vCard.erase(iter);
	}

	// sign ming gang info 
	VEC_INVOKE_ACT_INFO::value_type tMingGang;
	tMingGang.nCard = nCard;
	tMingGang.nInvokerIdx = nInvokerIdx;
	m_vDirectGanged.push_back(tMingGang);
	
	// new get card ;
	onMoCard(nGangGetCard);
	//debugCardInfo();
	return true;
}

bool MJPlayerCard::onAnGang(uint8_t nCard, uint8_t nGangGetCard)
{
	auto eType = card_Type(nCard);
	if (eType >= eCT_Max)
	{
		LOGFMTE("onAnGang parse card type error so do not have this card = %u", nCard);
		return false;
	}

	auto& vCard = m_vCards[eType];
	auto nEraseCnt = 4;
	while (nEraseCnt-- > 0)
	{
		auto iter = std::find(vCard.begin(), vCard.end(), nCard);
		vCard.erase(iter);
	}

	//addCardToVecAsc(m_vGanged, nCard);
	m_vAnGanged.push_back(nCard);
	// new get card ;
	onMoCard(nGangGetCard);
	//debugCardInfo();
	return true;
}

bool MJPlayerCard::onBuGang(uint8_t nCard, uint8_t nGangGetCard)
{
	auto eType = card_Type(nCard);
	if (eType >= eCT_Max)
	{
		LOGFMTE("onMingGang parse card type error so do not have this card = %u", nCard);
		return false;
	}

	// remove hold card 
	removeHoldCard(nCard);

	// remove peng 
	auto iterPeng = std::find_if(m_vPenged.begin(), m_vPenged.end(), [nCard](VEC_INVOKE_ACT_INFO::value_type& ref) { return ref.nCard == nCard; });
	if (iterPeng == m_vPenged.end())
	{
		LOGFMTE("not peng , hao to bu gang ? %u ",nCard); 
		return false;
	}
	// sign Bu gang info 
	m_vBuGang.push_back(*iterPeng);
	// erase peng 
	m_vPenged.erase(iterPeng);

	// new get card ;
	onMoCard(nGangGetCard);
	//debugCardInfo();
	return true;
}

bool MJPlayerCard::onEat(uint8_t nCard, uint8_t nWithA, uint8_t withB)
{
	if (!isHaveCard(nWithA) || !isHaveCard(withB))
	{
		LOGFMTE("do not have with card , can not eat a = %u , b = %u, c = %u",nCard,nWithA,withB);
		return false;
	}

	auto eT = card_Type(nCard);
	if (card_Type(nCard) != card_Type(nWithA))
	{
		LOGFMTE("not the same type , can not eat a = %u , b = %u, c = %u", nCard, nWithA, withB);
		return false;
	}

	removeHoldCard(nWithA);
	removeHoldCard(withB);

	// sign eat info 
	VEC_CARD v{ nCard  ,nWithA, withB };
	std::sort(v.begin(),v.end());
	// add to eat , should not sort 
	m_vEated.insert(m_vEated.end(),v.begin(),v.end());
	return true;
}

bool MJPlayerCard::onChuCard(uint8_t nChuCard)
{
	if (!isHaveCard(nChuCard))
	{
		LOGFMTE("you don't have this card , how can chu ?  = %u",nChuCard);
		return false;
	}
	auto eT = card_Type(nChuCard);
	auto iter = std::find(m_vCards[eT].begin(), m_vCards[eT].end(), nChuCard);
	m_vCards[eT].erase(iter);
	m_vChuedCard.push_back(nChuCard);

	//debugCardInfo();
	m_vLouPenged.clear();
	//clearFlag(ePlayerFlag_CanTianHu);
	//clearFlag( ePlayerFlag_WaitCheckTianTing );
	return true;
}

bool MJPlayerCard::getHoldCard(VEC_CARD& vHoldCard)
{
	for (auto& vCards : m_vCards)
	{
		if (vCards.empty())
		{
			continue;
		}
		vHoldCard.insert(vHoldCard.end(),vCards.begin(),vCards.end());
	}
	return !vHoldCard.empty();
}

bool MJPlayerCard::getChuedCard(VEC_CARD& vChuedCard)
{
	vChuedCard.insert(vChuedCard.end(),m_vChuedCard.begin(),m_vChuedCard.end());
	return vChuedCard.empty() == false;
}

bool MJPlayerCard::getAnGangedCard(VEC_CARD& vAnGanged)
{
	vAnGanged.insert(vAnGanged.end(), m_vAnGanged.begin(), m_vAnGanged.end());
	return vAnGanged.empty() == false;
}

bool MJPlayerCard::getMingGangedCard(VEC_CARD& vGangCard)
{
	getBuGangCard(vGangCard);
	VEC_CARD vCard;
	if (getDirectGangCard(vCard))
	{
		vGangCard.insert(vGangCard.end(),vCard.begin(),vCard.end());
	}
	return !vGangCard.empty();
}

bool MJPlayerCard::getDirectGangCard(VEC_CARD& vGangCard)
{
	for (auto& ref : m_vDirectGanged )
	{
		vGangCard.push_back(ref.nCard);
	}
	return !vGangCard.empty();
}

bool MJPlayerCard::getBuGangCard(VEC_CARD& vGangCard)
{
	for (auto& ref : m_vBuGang )
	{
		vGangCard.push_back(ref.nCard);
	}
	return !vGangCard.empty();
}

bool MJPlayerCard::getPengedCard(VEC_CARD& vPengedCard)
{
	for (auto& ref : m_vPenged )
	{
		vPengedCard.push_back(ref.nCard);
	}
	return !vPengedCard.empty();
}

bool MJPlayerCard::getEatedCard(VEC_CARD& vEatedCard)
{
	vEatedCard.insert(vEatedCard.end(), m_vEated.begin(), m_vEated.end());
	return false == vEatedCard.empty();
}

bool MJPlayerCard::isCardTypeMustKeZi(uint8_t nCardType)
{
	return nCardType == eCT_Jian || eCT_Feng == nCardType ;
}

uint32_t MJPlayerCard::getNewestFetchedCard()
{
	return m_nNesetFetchedCard;
}

void MJPlayerCard::addHoldCard( uint8_t nCard )
{
	auto nType = card_Type(nCard);
	if ( nType >= eCT_Max)
	{
		LOGFMTE( "invalid card type , card = %u , can not add holdCard",nCard );
		return;
	}

	VEC_CARD& vec = m_vCards[nType];
	auto iter = vec.begin();
	for (; iter < vec.end(); ++iter)
	{
		if ((*iter) >= nCard)
		{
			vec.insert(iter, nCard);
			return;
		}
	}
	vec.push_back(nCard);
}

void MJPlayerCard::removeHoldCard(uint8_t nCard)
{
	auto nType = card_Type(nCard);
	if (nType >= eCT_Max)
	{
		LOGFMTE("invalid card type , card = %u , can not add removeHoldCard", nCard);
		return;
	}

	VEC_CARD& vec = m_vCards[nType];
	auto iter = std::find(vec.begin(),vec.end(),nCard );
	if (iter != vec.end())
	{
		vec.erase( iter );
		return;
	}
	LOGFMTE( "do not have card = %u , can not removeHoldCard",nCard );
	return;
}

bool MJPlayerCard::isHoldCardCanHuNormal( uint8_t& Jiang )
{
	uint8_t nJiangType = eCT_Max;
	for (uint8_t nType = eCT_None; nType < eCT_Max; ++nType)
	{
		auto& vCards = m_vCards[nType];
		if ( vCards.size() % 3 == 2 && eCT_Max == nJiangType )
		{
			nJiangType = nType;
			continue;
		}

		if ( vCards.size() % 3 != 0 )
		{
			return false;
		}

		if ( isAllShunziOrKeZi(vCards) == false )
		{
			return false;
		}
	}

	if ( eCT_Max == nJiangType )
	{
		LOGFMTE("why this hold card do not have jiang ? xiang gang ? ");
		return false;
	}

	// find jiang ;
	auto vCheck = m_vCards[nJiangType];
	std::set<uint8_t> vJiang;
	for ( uint8_t nIdx = 0; (nIdx + 1) < vCheck.size();  )
	{
		if (vCheck[nIdx] == vCheck[nIdx + 1])
		{
			vJiang.insert(vCheck[nIdx]);
			nIdx += 2;
			continue;
		}
		++nIdx;
	}

	if ( vJiang.empty() )
	{
		return false;
	}

	for (auto ref : vJiang)
	{
		vCheck.clear();
		vCheck = m_vCards[nJiangType];
		
		auto iter = std::find(vCheck.begin(),vCheck.end(),ref);
		vCheck.erase(iter);
		std::find(vCheck.begin(), vCheck.end(), ref);
		vCheck.erase(iter);
		
		if (isAllShunziOrKeZi(vCheck))
		{
			Jiang = ref;
			return true;
		}
	}
	return false;
}

bool MJPlayerCard::isAllShunziOrKeZi( VEC_CARD vCards )
{
	if ( vCards.empty() )
	{
		return true;
	}

	if ( vCards.size() % 3 != 0 )
	{
		return false;
	}

	auto isMustKeZi = isCardTypeMustKeZi( card_Type(vCards.front()) );
	for ( uint8_t nIdx = 0; nIdx < vCards.size(); )
	{
		if (vCards[nIdx] == 0)
		{
			++nIdx;
			continue;
		}

		if ( vCards[nIdx] == vCards[nIdx + 2] )
		{
			vCards[nIdx] = vCards[nIdx+1] = vCards[nIdx+2] = 0;
			nIdx += 3;
			continue;
		}

		if ( isMustKeZi )
		{
			return false;
		}

		auto iterSecond = std::find( vCards.begin(),vCards.end(), vCards[nIdx] + 1 );
		auto iterThired = std::find(vCards.begin(), vCards.end(), vCards[nIdx] + 2);
		if ( iterSecond != vCards.end() && iterThired != vCards.end() )
		{
			vCards[nIdx] = 0;
			(*iterSecond) = 0;
			(*iterThired) = 0;
			++nIdx;
			continue;
		}
		return false;
	}

	return true;
}

bool MJPlayerCard::is7PairTing( uint8_t& nJiang )
{
	VEC_CARD vHold;
	getHoldCard(vHold);
	if (vHold.size() != 13)
	{
		return false;
	}
	std::sort(vHold.begin(),vHold.end());
	nJiang = 0;
	for (size_t nIdx = 0; (nIdx + 1) < vHold.size();  )
	{
		if ( vHold[nIdx] != vHold[nIdx + 1] )
		{
			if ( nJiang == 0 )
			{
				nJiang = vHold[nIdx];
				++nIdx;
				continue;
			}
			nJiang = 0;
			return false;
		}
		nIdx += 2;
	}
	return true;
}

bool MJPlayerCard::isNormalTing()
{
	std::set<uint8_t> vCanHus;
	return getNormalCanHuCards(vCanHus);
}

bool MJPlayerCard::getNormalCanHuCards(std::set<uint8_t>& vCanHus)
{
	std::vector<uint8_t> vNotShunKeZiType;
	for (uint16_t nType = eCT_None; nType < eCT_Max; ++nType)
	{
		VEC_CARD& vCards = m_vCards[nType];
		if (vCards.size() % 3 != 0)
		{
			vNotShunKeZiType.push_back(nType);
			continue;
		}

		if (vCards.size() % 3 == 0 && (false == isAllShunziOrKeZi(vCards)))
		{
			return false;
		}
	}

	if (vNotShunKeZiType.size() > 2)
	{
		return false;
	}

	if (vNotShunKeZiType.size() == 1)
	{
		auto nType = vNotShunKeZiType.front();
		auto vCards = m_vCards[nType];
		if (vCards.size() % 3 != 1)
		{
			LOGFMTE("you are xiang gang le ? ");
			debugCardInfo();
			return false;
		}
	}

	if (vNotShunKeZiType.size() == 2)
	{
		auto nType1 = vNotShunKeZiType[0];
		auto nType2 = vNotShunKeZiType[1];
		if (m_vCards[nType1].size() % 3 != 2 || 2 != m_vCards[nType2].size() % 3)
		{
			LOGFMTD("must two type should be 2");
			return false;
		}
	}

	// check can hu card ;
	for (auto &nTingType : vNotShunKeZiType)
	{
		auto nType = card_Type(nTingType);
		auto nCnt = 9;
		if (eCT_Jian == nType)
		{
			nCnt = 3;
		}
		else if (eCT_Feng == nType)
		{
			nCnt = 4;
		}

		uint8_t nCard = make_Card_Num(nType, 1);
		uint8_t nJiang = 0;
		while (nCnt--)
		{
			addHoldCard(nCard);
			auto isHu = isHoldCardCanHuNormal(nJiang);
			removeHoldCard(nCard);
			if (isHu)
			{
				vCanHus.insert(nCard);
			}
			++nCard;
		}
	}

	return vCanHus.empty() == false;
}

bool MJPlayerCard::canHoldCard7PairHu()
{
	VEC_CARD vHold;
	getHoldCard(vHold);
	if (vHold.size() != 14)
	{
		return false;
	}
	std::sort(vHold.begin(), vHold.end());
	for (size_t nIdx = 0; (nIdx + 1) < vHold.size(); nIdx += 2)
	{
		if (vHold[nIdx] != vHold[nIdx + 1])
		{
			return false;
		}
	}
	return false;
}

void MJPlayerCard::addLouPengedCard(uint8_t nLouPengedCard)
{
	m_vLouPenged.push_back(nLouPengedCard);
}

bool MJPlayerCard::getCanHuCards(std::set<uint8_t>& vCanHus)
{
	uint8_t nJiang = 0;
	if (is7PairTing(nJiang))
	{
		vCanHus.insert(nJiang);
		return true;
	}
	return getNormalCanHuCards(vCanHus);
}

uint8_t MJPlayerCard::getJiang()
{
	if (canHoldCard7PairHu())
	{
		return getNewestFetchedCard();
	}

	uint8_t nJiang = 0;
	if ( !isHoldCardCanHuNormal(nJiang) )
	{
		LOGFMTE( "hold card is not hu , can not find jiang " );
		return 0;
	}
	return nJiang;
}
