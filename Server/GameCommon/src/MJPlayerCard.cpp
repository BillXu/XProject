#include "MJPlayerCard.h"
#include "IMJPoker.h"
#include "log4z.h"
#include "ServerCommon.h"
// mj player card ;
void MJPlayerCard::reset()
{
	for (auto& vC : m_vCards)
	{
		vC.clear();
	}
	m_vChuedCard.clear();
	m_vMingCardInfo.clear();
	m_nNesetFetchedCard = 0 ;
	m_nJIang = 0;
	m_vLouPenged.clear();
	m_nDianPaoIdx = 0;
}

void MJPlayerCard::onDoHu(uint16_t nInvokerIdx, uint8_t nHuCard, bool isInvokerHaveGangFlag)
{
	m_nDianPaoIdx = nInvokerIdx;
	onMoCard(nHuCard);
	uint8_t nJiang = 0;
	if (!isHoldCardCanHu(nJiang))
	{
		return;
		LOGFMTE( "you can not hu , why do hu ?" );
	}
	m_nJIang = nJiang;
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

bool MJPlayerCard::canEatCard(uint8_t nCard, uint8_t nWith1, uint8_t nWith2)
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

	if (nWith1 && nWith2) {
		auto tType = card_Type(nWith1);
		if (tType != eType || isHaveCard(nWith1) == false) {
			LOGFMTE("canEatCard parse card type error with card = %u", nWith1);
			return false;
		}

		tType = card_Type(nWith2);
		if (tType != eType || isHaveCard(nWith2) == false) {
			LOGFMTE("canEatCard parse card type error with card = %u", nWith2);
			return false;
		}

		std::vector<uint8_t> vt_Cards;
		vt_Cards.push_back(nCard);
		vt_Cards.push_back(nWith1);
		vt_Cards.push_back(nWith2);

		std::sort(vt_Cards.begin(), vt_Cards.end());
		if (vt_Cards[0] + 1 == vt_Cards[1] && vt_Cards[1] + 1 == vt_Cards[2]) {
			return true;
		}
		LOGFMTE("canEatCard error can not eat card = %u, with card = %u and card = %u", nCard, nWith1, nWith2);
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

bool MJPlayerCard::canHuWitCard( uint8_t nCard )
{
	auto eType = card_Type(nCard);
	if (eType >= eCT_Max)
	{
		LOGFMTE("parse card type error ,canHuWitCard have this card = %u", nCard);
		return false;
	}

	addHoldCard(nCard);
	uint8_t nJiang = 0;
	bool bSelfHu = isHoldCardCanHu(nJiang);
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

	return m_vMingCardInfo.end() != std::find_if(m_vMingCardInfo.begin(), m_vMingCardInfo.end(), [nCard](VEC_INVOKE_ACT_INFO::value_type& ref) { return ref.nTargetCard == nCard && ref.eAct == eMJAct_Peng; });
}

bool MJPlayerCard::isTingPai()
{
	uint8_t nBaiDaCnt = 0;
	if (getBaiDaCard() > 0) // enabled bai Da card ;
	{
		VEC_CARD vHolds;
		getHoldCard(vHolds);
		nBaiDaCnt = std::count(vHolds.begin(), vHolds.end(), getBaiDaCard());
	}

	// remove bai da card from hold 
	auto nBaiCard = getBaiDaCard();
	while (nBaiCard > 0 && isHaveCard(nBaiCard))
	{
		removeHoldCard(nBaiCard);
	}

	// check ting , means 13 cards + one bai da ;
	++nBaiDaCnt; // check ting bai da jia yi ;
	bool isTing = false;
	uint8_t nJiang = 0;
	if (isEanble7Pair())
	{
		isTing = canHoldCard7PairHu(nJiang, nBaiDaCnt);
	}

	if (!isTing)
	{
		isTing = isHoldCardCanHuNormal(nJiang, nBaiDaCnt);
	}
	--nBaiDaCnt; // after check ting  ; 
	// add back baiDa to hold 
	while (nBaiDaCnt--)
	{
		addHoldCard(nBaiCard);
	}
	return isTing;
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

void MJPlayerCard::onVisitPlayerCardInfo(Json::Value& js, bool isSelf)
{
	// ming card ;
	Json::Value jsMing;
	for ( auto& ref : m_vMingCardInfo )
	{
		Json::Value jsItem, jsCards;
		jsItem["act"] = ref.eAct;
		jsCards[jsCards.size()] = ref.nTargetCard;
		if (ref.eAct == eMJAct_Chi)
		{
			for (auto& chi : ref.vWithCard)
			{
				jsCards[jsCards.size()] = chi;
			}
		}
		jsItem["card"] = jsCards;
		jsItem["invokerIdx"] = ref.nInvokerIdx;
		jsMing[jsMing.size()] = jsItem;
	}
	js["ming"] = jsMing;
	Json::Value jsChued;
	for (auto& ref : m_vChuedCard) {
		jsChued[jsChued.size()] = ref;
	}
	js["chued"] = jsChued;

	// hold ;
	VEC_CARD vHold;
	getHoldCard(vHold);
	js["holdCnt"] = vHold.size();
	if ( isSelf )
	{
		Json::Value jsHolds;
		for (auto& refHold : vHold )
		{
			jsHolds[jsHolds.size()] = refHold;
		}
		js["holdCards"] = jsHolds;
	}
}

void MJPlayerCard::onVisitPlayerCardBaseInfo(Json::Value& js) {
	// ming card ;
	Json::Value jsMing;
	for (auto& ref : m_vMingCardInfo)
	{
		Json::Value jsItem, jsCards;
		jsItem["act"] = ref.eAct;
		jsCards[jsCards.size()] = ref.nTargetCard;
		if (ref.eAct == eMJAct_Chi)
		{
			for (auto& chi : ref.vWithCard)
			{
				jsCards[jsCards.size()] = chi;
			}
		}
		jsItem["card"] = jsCards;
		jsItem["invokerIdx"] = ref.nInvokerIdx;
		jsMing[jsMing.size()] = jsItem;
	}
	js["ming"] = jsMing;

	// hold ;
	Json::Value jsHolds;
	VEC_CARD vHold;
	getHoldCard(vHold);
	for (auto& refHold : vHold)
	{
		jsHolds[jsHolds.size()] = refHold;
	}
	js["holdCards"] = jsHolds;
}

bool MJPlayerCard::getHoldCardThatCanBuGang(VEC_CARD& vGangCards)
{
	for (auto& ref : m_vMingCardInfo)
	{
		if ( ref.eAct != eMJAct_Peng)
		{
			continue;
		}

		if (isHaveCard(ref.nTargetCard))
		{
			vGangCards.push_back(ref.nTargetCard );
		}
	}

	return !vGangCards.empty();
}

bool MJPlayerCard::isHoldCardCanHu( uint8_t& nJiang )
{
	m_b7Pair = false;
	uint8_t nBaiDaCnt = 0;
	if ( getBaiDaCard() > 0 ) // enabled bai Da card ;
	{
		VEC_CARD vHolds;
		getHoldCard(vHolds);
		nBaiDaCnt = std::count(vHolds.begin(), vHolds.end(), getBaiDaCard());
	}

	// remove bai da card from hold 
	auto nBaiCard = getBaiDaCard();
	while ( nBaiCard > 0 && isHaveCard(nBaiCard) )
	{
		removeHoldCard(nBaiCard);
	}

	// check hu
	bool isHu = false;
	if ( isEanble7Pair() )
	{
		isHu = canHoldCard7PairHu(nJiang,nBaiDaCnt);
	}

	if ( !isHu )
	{
		isHu = isHoldCardCanHuNormal(nJiang, nBaiDaCnt);
	}
	 
	// add back baiDa to hold 
	while (nBaiDaCnt--)
	{
		addHoldCard(nBaiCard);
	}
	return isHu;
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
	tPeng.nTargetCard = nCard;
	tPeng.nInvokerIdx = (uint8_t)nInvokerIdx;
	tPeng.eAct = eMJAct_Peng;
	m_vMingCardInfo.push_back(tPeng);
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
	tMingGang.nTargetCard = nCard;
	tMingGang.nInvokerIdx = (uint8_t)nInvokerIdx;
	tMingGang.eAct = eMJAct_MingGang;
	m_vMingCardInfo.push_back(tMingGang);
	
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
	VEC_INVOKE_ACT_INFO::value_type tAnGang;
	tAnGang.nTargetCard = nCard;
	tAnGang.nInvokerIdx = 0;
	tAnGang.eAct = eMJAct_AnGang;
	m_vMingCardInfo.push_back(tAnGang);
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
	auto iterPeng = std::find_if(m_vMingCardInfo.begin(), m_vMingCardInfo.end(), [nCard](VEC_INVOKE_ACT_INFO::value_type& ref) { return ref.nTargetCard == nCard && ref.eAct == eMJAct_Peng; });
	if (iterPeng == m_vMingCardInfo.end())
	{
		LOGFMTE("not peng , hao to bu gang ? %u ",nCard); 
		return false;
	}
	// sign Bu gang info 
	iterPeng->eAct = eMJAct_BuGang;

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
	VEC_INVOKE_ACT_INFO::value_type tEatInfo;
	tEatInfo.nTargetCard = nCard;
	tEatInfo.nInvokerIdx = 0;
	tEatInfo.eAct = eMJAct_Chi;
	tEatInfo.vWithCard.push_back(nWithA);
	tEatInfo.vWithCard.push_back(withB);
	m_vMingCardInfo.push_back(tEatInfo);
	return true;
}

bool MJPlayerCard::onChuCard(uint8_t nChuCard)
{
	if (!isHaveCard(nChuCard))
	{
		LOGFMTE("you don't have this card , how can chu ?  = %u",nChuCard);
		return false;
	}
	/*auto eT = card_Type(nChuCard);
	auto iter = std::find(m_vCards[eT].begin(), m_vCards[eT].end(), nChuCard);
	m_vCards[eT].erase(iter);*/
	removeHoldCard(nChuCard);
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
	for (auto ref : m_vMingCardInfo)
	{
		if (ref.eAct == eMJAct_AnGang)
		{
			vAnGanged.push_back(ref.nTargetCard);
		}
	}
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
	for (auto ref : m_vMingCardInfo)
	{
		if (ref.eAct == eMJAct_MingGang)
		{
			vGangCard.push_back(ref.nTargetCard);
		}
	}
	return !vGangCard.empty();
}

bool MJPlayerCard::getBuGangCard(VEC_CARD& vGangCard)
{
	for (auto ref : m_vMingCardInfo)
	{
		if (ref.eAct == eMJAct_BuGang )
		{
			vGangCard.push_back(ref.nTargetCard);
		}
	}
	return !vGangCard.empty();
}

bool MJPlayerCard::getPengedCard(VEC_CARD& vPengedCard)
{
	for (auto ref : m_vMingCardInfo)
	{
		if (ref.eAct == eMJAct_Peng )
		{
			vPengedCard.push_back(ref.nTargetCard);
		}
	}
	return !vPengedCard.empty();
}

bool MJPlayerCard::getEatedCard(VEC_CARD& vEatedCard)
{
	VEC_CARD vChi;
	for (auto ref : m_vMingCardInfo)
	{
		if (ref.eAct == eMJAct_Chi )
		{
			vChi = ref.vWithCard;
			vChi.push_back(ref.nTargetCard);

			std::sort(vChi.begin(), vChi.end());
			vEatedCard.insert(vEatedCard.end(), vChi.begin(), vChi.end());
			vChi.clear();
		}
	}
	
	return false == vEatedCard.empty();
}

bool MJPlayerCard::getBuHuaCard(VEC_CARD& vHuaCard) {
	for (auto ref : m_vMingCardInfo)
	{
		if (ref.eAct == eMJAct_BuHua)
		{
			vHuaCard.push_back(ref.nTargetCard);
		}
	}
	return !vHuaCard.empty();
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

	auto nValue = card_Value(nCard);
	uint8_t nMaxValue = 9;
	if (nType == eCT_Feng) {
		nMaxValue = 4;
	}
	else if (nType == eCT_Jian) {
		nMaxValue = 3;
	}
	Assert(nValue >= 1 && nValue <= nMaxValue, "invalid card type");

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
	Assert(false, "invalid card type");
	return;
}

bool MJPlayerCard::CheckHoldCardAllShun(uint8_t nBaiDaCnt) {
	VEC_CARD vNotShun;
	uint8_t nAlreadyQueCnt = 0;
	for (uint8_t nType = eCT_None; nType < eCT_Max; ++nType)
	{
		uint8_t nQueCnt = 0;
		tryToFindMiniQueCnt(m_vCards[nType], isCardTypeMustKeZi(nType), vNotShun, nQueCnt, nBaiDaCnt - nAlreadyQueCnt);
		nAlreadyQueCnt += nQueCnt;
		if (nAlreadyQueCnt > nBaiDaCnt)
		{
			return false;
		}
	}
	return true;
}

bool MJPlayerCard::isHoldCardCanHuNormal( uint8_t& nJiang, uint8_t nBaiDaCnt )
{
	// check all hold card is hun , ignore jiang ;
	/*auto pCheckHoldCardAllShun = [this](uint8_t nBaiDaCnt)
	{
		VEC_CARD vNotShun;
		uint8_t nAlreadyQueCnt = 0;
		for (uint8_t nType = eCT_None; nType < eCT_Max; ++nType)
		{
			uint8_t nQueCnt = 0;
			tryToFindMiniQueCnt(m_vCards[nType], isCardTypeMustKeZi(nType), vNotShun, nQueCnt, nBaiDaCnt - nAlreadyQueCnt );
			nAlreadyQueCnt += nQueCnt;
			if ( nAlreadyQueCnt > nBaiDaCnt)
			{
				return false;
			}
		}
		return true;
	};*/
 
	// when baiDa not Be Jiang  
	// find maybe jiang 
	VEC_CARD vHoldCards;
	getHoldCard(vHoldCards);

	std::set<uint8_t> vMayBeJiang;
	for (uint8_t nIdx = 0; (nIdx + 1) < vHoldCards.size(); )
	{
		if (vHoldCards[nIdx] == vHoldCards[nIdx + 1])
		{
			vMayBeJiang.insert(vHoldCards[nIdx]);
			nIdx += 2;
			continue;
		}
		++nIdx;
	}

	for ( auto& ref : vMayBeJiang )
	{
		// remove jiang ;
		removeHoldCard(ref);
		removeHoldCard(ref);
		// check all card type is shun, without jiang ;
		bool bOk = CheckHoldCardAllShun(nBaiDaCnt);
		// add back jiang 
		addHoldCard(ref);
		addHoldCard(ref);

		if (bOk)  // ok this jiang can hu , so need not go on try other jiang 
		{
			nJiang = ref;
			return true;
		}
	}

	// 2 baiDa as jiang 
	if ( nBaiDaCnt >= 2 ) 
	{
		if (CheckHoldCardAllShun(nBaiDaCnt - 2 ))
		{
			nJiang = 0; // zero means baiDa as jiang ;
			return true;
		}
	}

	// one baiDa as Jiang 
	if ( nBaiDaCnt >= 1 ) 
	{
		vMayBeJiang.clear();
		vMayBeJiang.insert(vHoldCards.begin(),vHoldCards.end());  // erase duplicate cards ;
		for (auto& ref : vMayBeJiang)
		{
			// remove jiang ;
			removeHoldCard(ref);
			// check all card type is shun, without jiang ;
			bool bOk = CheckHoldCardAllShun( nBaiDaCnt - 1 );
			// add back jiang 
			addHoldCard(ref);

			if (bOk)  // ok this jiang can hu , so need not go on try other jiang 
			{
				nJiang = ref;
				return true;
			}
		}
	}

	return false;
}

bool MJPlayerCard::canHoldCard7PairHu( uint8_t& nJiang, uint8_t nBaiDaCnt )
{
	VEC_CARD vHold;
	getHoldCard(vHold);
	if ( (vHold.size() + nBaiDaCnt ) != 14 )
	{
		return false;
	}
	std::sort(vHold.begin(), vHold.end());
	for (size_t nIdx = 0; (nIdx + 1) < vHold.size(); )
	{
		if ( vHold[nIdx] != vHold[nIdx + 1] )
		{
			if (nBaiDaCnt >= 1)
			{
				++nIdx;
				--nBaiDaCnt;
				continue;
			}
			return false;
		}
		nIdx += 2;
	}

	nJiang = getNewestFetchedCard();
	m_b7Pair = true;
	return true;
}

void MJPlayerCard::addLouPengedCard(uint8_t nLouPengedCard)
{
	m_vLouPenged.push_back(nLouPengedCard);
}

uint8_t MJPlayerCard::getJiang()
{
	uint8_t nJiang = 0;
	isHoldCardCanHu(nJiang);
	return nJiang;
}

void MJPlayerCard::debugCardInfo()
{
	return;
	LOGFMTD("card Info: \n");
	VEC_CARD vHold;
	getHoldCard(vHold);
	for (uint8_t nCard : vHold )
	{
		LOGFMTD("cardNumber : %u\n", nCard);
	}
	LOGFMTD("card info end \n\n");
}

uint8_t MJPlayerCard::tryToFindMiniQueCnt(VEC_CARD vWaitCheck, bool isMustKeZi, VEC_CARD& vNotShun, uint8_t& nQueCnt, uint8_t nMaxQueCnt)
{
	if (vWaitCheck.empty())
	{
		return nQueCnt;
	}

	auto pRemoveZero = [](VEC_CARD& vCard)
	{
		do
		{
			auto iter = std::find(vCard.begin(), vCard.end(), 0);
			if (iter == vCard.end())
			{
				return;
			}
			vCard.erase(iter);
		} while (1);
	};

	std::vector<uint8_t> vThisBestNotShun;
	uint8_t nThisBestQue = 100;
	// as ke zi ;
	if (vWaitCheck.size() >= 3)
	{
		if (vWaitCheck[0] == vWaitCheck[2])
		{
			// do check ke  zi ;
			VEC_CARD vCards = vWaitCheck;
			vCards[0] = vCards[1] = vCards[2] = 0;
			pRemoveZero(vCards);

			VEC_CARD vTemp = vNotShun;
			uint8_t nTempQue = nQueCnt;
			tryToFindMiniQueCnt(vCards, isMustKeZi, vTemp, nTempQue, nMaxQueCnt);
			if (nTempQue < nThisBestQue)
			{
				nThisBestQue = nTempQue;
				if (0 == nThisBestQue)
				{
					return 0;
				}
				vThisBestNotShun = vTemp;
			}
		}
	}

	if (vWaitCheck.size() >= 2)
	{
		if (vWaitCheck[0] == vWaitCheck[1] && (nQueCnt + 1) <= nMaxQueCnt)
		{
			// do check ke  zi ;
			uint8_t nTempQue = nQueCnt + 1;

			VEC_CARD vTemp = vNotShun;
			vTemp.push_back(vWaitCheck[0]);
			vTemp.push_back(vWaitCheck[1]);

			VEC_CARD vCards = vWaitCheck;
			vCards[0] = vCards[1] = 0;
			pRemoveZero(vCards);

			tryToFindMiniQueCnt(vCards, isMustKeZi, vTemp, nTempQue, nMaxQueCnt);
			if (nTempQue < nThisBestQue)
			{
				nThisBestQue = nTempQue;
				if (0 == nThisBestQue)
				{
					return 0;
				}
				vThisBestNotShun = vTemp;
			}
		}
	}

	// as shun zi ;
	if (false == isMustKeZi && vWaitCheck.size() >= 3 && card_Value(vWaitCheck[0]) <= 7)
	{
		VEC_CARD vCards = vWaitCheck;
		auto iter2 = std::find(vCards.begin(), vCards.end(), vCards[0] + 1);
		auto iter3 = std::find(vCards.begin(), vCards.end(), vCards[0] + 2);
		if (iter2 != vCards.end() && iter3 != vCards.end())
		{
			vCards[0] = 0;
			*iter2 = 0; *iter3 = 0;

			pRemoveZero(vCards);

			uint8_t nTempQue = nQueCnt;
			VEC_CARD vTemp = vNotShun;
			tryToFindMiniQueCnt(vCards, isMustKeZi, vTemp, nTempQue, nMaxQueCnt);
			if (nTempQue < nThisBestQue)
			{
				nThisBestQue = nTempQue;
				if (0 == nThisBestQue)
				{
					return 0;
				}
				vThisBestNotShun = vTemp;
			}
		}
	}

	if (false == isMustKeZi && vWaitCheck.size() >= 2 && card_Value(vWaitCheck[0]) <= 8 && (nQueCnt + 1) <= nMaxQueCnt)
	{
		VEC_CARD vCards = vWaitCheck;
		auto iter2 = std::find(vCards.begin(), vCards.end(), vCards[0] + 1);
		auto iter3 = std::find(vCards.begin(), vCards.end(), vCards[0] + 2);
		auto& iterNotNull = iter2 != vCards.end() ? iter2 : iter3;
		if (iterNotNull != vCards.end())
		{
			// do check ke  zi ;
			uint8_t nTempQue = nQueCnt + 1;

			VEC_CARD vTemp = vNotShun;
			vTemp.push_back(vCards[0]);
			vTemp.push_back(*iterNotNull);

			vCards[0] = 0;
			*iterNotNull = 0;
			pRemoveZero(vCards);

			tryToFindMiniQueCnt(vCards, isMustKeZi, vTemp, nTempQue, nMaxQueCnt);
			if (nTempQue < nThisBestQue)
			{
				nThisBestQue = nTempQue;
				if (0 == nThisBestQue)
				{
					return 0;
				}
				vThisBestNotShun = vTemp;
			}
		}
	}

	// as single ;
	if ((nQueCnt + 2) <= nMaxQueCnt)
	{
		VEC_CARD vCards = vWaitCheck;

		uint8_t nTempQue = nQueCnt + 2;

		VEC_CARD vTemp = vNotShun;
		vTemp.push_back(vCards[0]);

		vCards[0] = 0;
		pRemoveZero(vCards);

		tryToFindMiniQueCnt(vCards, isMustKeZi, vTemp, nTempQue, nMaxQueCnt);
		if (nTempQue < nThisBestQue)
		{
			nThisBestQue = nTempQue;
			if (0 == nThisBestQue)
			{
				return 0;
			}
			vThisBestNotShun = vTemp;
		}
	}

	nQueCnt = nThisBestQue;
	if (nQueCnt < 100)
	{
		vNotShun = vThisBestNotShun;
	}
	return nQueCnt;
}

bool MJPlayerCard::onBuHua(uint8_t nHuaCard, uint8_t nGetCard) {
	if (!isHaveCard(nHuaCard))
	{
		LOGFMTE("you don't have this card , how can bu hua ?  = %u", nHuaCard);
		return false;
	}

	// remove hold card 
	removeHoldCard(nHuaCard);

	//addCardToVecAsc(m_vGanged, nCard);
	VEC_INVOKE_ACT_INFO::value_type tBuHua;
	tBuHua.nTargetCard = nHuaCard;
	tBuHua.nInvokerIdx = 0;
	tBuHua.eAct = eMJAct_BuHua;
	m_vMingCardInfo.push_back(tBuHua);

	// new get card ;
	onMoCard(nGetCard);

	//debugCardInfo();
	return true;
}

uint8_t MJPlayerCard::getHuaCard() {
	if (m_vCards[eCT_Jian].size()) {
		return m_vCards[eCT_Jian].front();
	}
	else if (m_vCards[eCT_Hua].size()) {
		return m_vCards[eCT_Hua].front();
	}
	return 0;
}