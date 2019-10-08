#include "SDMJPlayerCard.h"
#include "IMJPoker.h"
#include "log4z.h"
#include "FanxingHunYiSe.h"
#include "FanxingQingYiSe.h"
#include "FanxingDuiDuiHu.h"
#include "Fanxing7Dui.h"
#include "SDMJPlayer.h"
#include "SDMJRoom.h"

SDMJPlayerCard::SDMJPlayerCard(){
	m_nBaiDaCard = make_Card_Num(eCT_Hua, 9);
}

void SDMJPlayerCard::reset() {
	MJPlayerCard::reset();
	m_nHuCard = 0;
	m_bChuDa = false;
}

bool SDMJPlayerCard::canPengWithCard(uint8_t nCard) {
	if (isChuDa()) {
		return false;
	}

	if (nCard == getBaiDaCard()) {
		return false;
	}

	return MJPlayerCard::canPengWithCard(nCard);
}

bool SDMJPlayerCard::canMingGangWithCard(uint8_t nCard) {
	if (isChuDa()) {
		return false;
	}

	if (nCard == getBaiDaCard()) {
		return false;
	}

	return MJPlayerCard::canMingGangWithCard(nCard);
}

bool SDMJPlayerCard::canAnGangWithCard(uint8_t nCard) {
	if (isChuDa()) {
		return false;
	}

	if (nCard == getBaiDaCard()) {
		return false;
	}

	return MJPlayerCard::canAnGangWithCard(nCard);
}

bool SDMJPlayerCard::canBuGangWithCard(uint8_t nCard) {
	if (isChuDa()) {
		return false;
	}

	if (nCard == getBaiDaCard()) {
		return false;
	}

	return MJPlayerCard::canBuGangWithCard(nCard);
}

bool SDMJPlayerCard::getHoldCardThatCanAnGang(VEC_CARD& vGangCards) {
	if (isChuDa()) {
		return false;
	}

	for (auto& vCard : m_vCards)
	{
		if (vCard.size() < 4)
		{
			continue;
		}

		for (uint8_t nIdx = 0; (uint8_t)(nIdx + 3) < vCard.size();)
		{
			if (vCard[nIdx] == vCard[nIdx + 3])
			{
				if (vCard[nIdx] != getBaiDaCard()) {
					vGangCards.push_back(vCard[nIdx]);
				}
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

bool SDMJPlayerCard::getHoldCardThatCanBuGang(VEC_CARD& vGangCards) {
	if (isChuDa()) {
		return false;
	}

	for (auto& ref : m_vMingCardInfo)
	{
		if (ref.eAct != eMJAct_Peng)
		{
			continue;
		}

		if (isHaveCard(ref.nTargetCard) && ref.nTargetCard != getBaiDaCard())
		{
			vGangCards.push_back(ref.nTargetCard);
		}
	}

	return !vGangCards.empty();
}

bool SDMJPlayerCard::canHuWitCard(uint8_t nCard) {
	return canHuWitCard(nCard, false);
}

bool SDMJPlayerCard::canHuWitCard(uint8_t nCard, bool isRotGang) {
	//出过百搭不能胡
	if (isChuDa()) {
		return false;
	}

	auto eType = card_Type(nCard);
	if (eType >= eCT_Max)
	{
		LOGFMTE("parse card type error ,canHuWitCard have this card = %u", nCard);
		return false;
	}

	auto pPlayer = (SDMJPlayer*)getPlayer();
	auto pRoom = (SDMJRoom*)pPlayer->getRoom();

	//不是抢杠只能自摸不能胡
	if (isRotGang == false && pRoom->isEnableOnlyZM()) {
		return false;
	}

	//百搭不能胡
	if (getBaiDaCard() == nCard) {
		return false;
	}

	//手里有百搭不能胡
	if (isHaveDa()) {
		//抢杠可以胡
		if (isRotGang == false) {
			return false;
		}
		//绕搭可以胡
		else if (checkRaoDa(false, nCard)) {
			return false;
		}
	}

	addHoldCard(nCard);
	setHuCard(nCard);
	bool bSelfHu = MJPlayerCard::isHoldCardCanHu(m_nJIang);
	if (bSelfHu) {
		if (m_b7Pair == false) {
			auto nHuaCnt = getHuaCntWithoutHuTypeHuaCnt();

			/* 2018/3/5
			修改无搭算硬花 计5花 客户端依然显示番型无搭+5花
			*/
			if (!isHaveDa()) {
				nHuaCnt += 5;
			}

			if (nHuaCnt + 1 < getDianPaoHuHuaRequire() && isRotGang) // not enought hua cnt ;
			{
				bSelfHu = checkDaMenQing() || checkXiaoMenQing() || checkHunYiSe() || checkQingYiSe() || checkDuiDuiHu() || ((SDMJPlayer*)getPlayer())->haveFlag(IMJPlayer::eMJActFlag_WaitCheckTianTing);
			}
		}
	}
	removeHoldCard(nCard);
	setHuCard(0);
	//debugCardInfo();
	return bSelfHu;
}

void SDMJPlayerCard::onDoHu(uint16_t nInvokerIdx, uint8_t nHuCard, bool isInvokerHaveGangFlag) {
	m_nDianPaoIdx = nInvokerIdx;
	onMoCard(nHuCard);
	if (!MJPlayerCard::isHoldCardCanHu(m_nJIang))
	{
		return;
		LOGFMTE("you can not hu , why do hu ?");
	}
	return;
}

bool SDMJPlayerCard::isHoldCardCanHu(uint8_t& nJiang) {
	//TODO
	return isHoldCardCanHu(nJiang, false);
}

bool SDMJPlayerCard::isHoldCardCanHu(uint8_t& nJiang, bool isGangKai) {
	if (isChuDa()) {
		return false;
	}

	bool bSelfHu = MJPlayerCard::isHoldCardCanHu(nJiang);
	if (bSelfHu)
	{
		if (m_b7Pair == false) {
			m_nJIang = nJiang;
			auto nHuaCnt = getHuaCntWithoutHuTypeHuaCnt();
			if (nHuaCnt < getZiMoHuaRequire()) // not enought hua cnt ;
			{
				bSelfHu = isGangKai || isHaveDa() == 0 ||
					((SDMJPlayer*)getPlayer())->haveFlag(IMJPlayer::eMJActFlag_CanTianHu) ||
					checkDaMenQing() || checkXiaoMenQing() ||
					checkHunYiSe() || checkQingYiSe() ||
					checkDuiDuiHu();
			}
		}
	}
	//debugCardInfo();
	return bSelfHu;
}

bool SDMJPlayerCard::isHaveCards(VEC_CARD vCards) {
	VEC_CARD vTemp;
	getHoldCard(vTemp);
	for (auto ref : vCards) {
		if (eraseVector(ref, vTemp)) {
			continue;
		}
		return false;
	}
	return true;
}

bool SDMJPlayerCard::eraseVector(uint8_t p, VEC_CARD& typeVec)
{
	VEC_CARD::iterator it = find(typeVec.begin(), typeVec.end(), p);
	if (it != typeVec.end())
	{
		typeVec.erase(it);
		return true;
	}
	return false;
}

uint8_t SDMJPlayerCard::getHoldCardCnt() {
	VEC_CARD vHoldCard;
	getHoldCard(vHoldCard);
	return vHoldCard.size();
}

uint8_t SDMJPlayerCard::getHuaCard() {
	if (m_vCards[eCT_Jian].size()) {
		return m_vCards[eCT_Jian].front();
	}
	else if (m_vCards[eCT_Hua].size()) {
		for (auto tCard : m_vCards[eCT_Hua]) {
			if (9 != card_Value(tCard)) {
				return tCard;
			}
		}
	}
	return 0;
}

uint8_t SDMJPlayerCard::getBaiDaCard() {
	return m_nBaiDaCard;
}

bool SDMJPlayerCard::onChuCard(uint8_t nChuCard) {
	bool bChuResult = MJPlayerCard::onChuCard(nChuCard);
	if (bChuResult) {
		if (nChuCard == getBaiDaCard()) {
			m_bChuDa = true;
		}
	}
	return bChuResult;
}

uint8_t SDMJPlayerCard::getHuaCntWithoutHuTypeHuaCnt(bool checkDMQ, stHuDetail* pHuDetail) {
	VEC_CARD vCard, vHuaCard;
	getBuHuaCard(vHuaCard);
	uint8_t nHuaCnt = vHuaCard.size();
	// check ming gang 
	getMingGangedCard(vCard);
	for (auto& ref : vCard)
	{
		if (card_Type(ref) == eCT_Feng)
		{
			nHuaCnt += 3;
		}
		else
		{
			nHuaCnt += 1;
		}
	}
	// check an gang 
	vCard.clear();
	getAnGangedCard(vCard);
	for (auto& ref : vCard)
	{
		if (card_Type(ref) == eCT_Feng)
		{
			nHuaCnt += 4;
		}
		else
		{
			nHuaCnt += 2;
		}
	}
	// check feng peng 
	vCard.clear();
	getPengedCard(vCard);
	for (auto& ref : vCard)
	{
		if (card_Type(ref) == eCT_Feng)
		{
			nHuaCnt += 1;
		}
	}

	if (checkDMQ && nHuaCnt) {
		return nHuaCnt;
	}

	// check feng an ke 
	//update by haodi
	nHuaCnt = checkAnKe(nHuaCnt, checkDMQ, pHuDetail);

	return nHuaCnt;
}

bool SDMJPlayerCard::checkDaMenQing() {
	return getHuaCntWithoutHuTypeHuaCnt(true) == 0;
}

bool SDMJPlayerCard::checkXiaoMenQing() {
	VEC_CARD vCard;
	getMingGangedCard(vCard);
	if (vCard.empty() == false)
	{
		return false;
	}

	getPengedCard(vCard);
	if (vCard.empty() == false)
	{
		//update by haodi 修改规则：大门清没有硬花碰牌算小门清 check暗刻已经在检测硬花中做过
		//auto n = getHuaCntWithoutHuTypeHuaCnt(nHuCard);
		//if (n != 0) {
		return false;
		//}
		//return false;
	}
	return true;
}

bool SDMJPlayerCard::checkHunYiSe() {
	VEC_CARD vAllCard;
	getHoldCard(vAllCard);

	daFilter(vAllCard);

	VEC_CARD vTemp;
	getAnGangedCard(vTemp);
	vAllCard.insert(vAllCard.end(), vTemp.begin(), vTemp.end());

	vTemp.clear();
	getMingGangedCard(vTemp);
	vAllCard.insert(vAllCard.end(), vTemp.begin(), vTemp.end());

	vTemp.clear();
	getPengedCard(vTemp);
	vAllCard.insert(vAllCard.end(), vTemp.begin(), vTemp.end());

	bool bFindFeng = false;
	do
	{
		auto iter = std::find_if(vAllCard.begin(), vAllCard.end(), [](uint8_t n) { return card_Type(n) == eCT_Feng; });
		if (iter != vAllCard.end())
		{
			bFindFeng = true;
			vAllCard.erase(iter);
		}
		else
		{
			break;
		}
	} while (1);

	if (bFindFeng == false || vAllCard.empty())
	{
		return false;
	}

	auto nType = card_Type(vAllCard.front());
	for (auto& ref : vAllCard)
	{
		auto tt = card_Type(ref);
		if (nType != tt)
		{
			return false;
		}
	}
	return true;
}

bool SDMJPlayerCard::checkQingYiSe() {
	VEC_CARD vAllCard;
	getHoldCard(vAllCard);

	daFilter(vAllCard);

	VEC_CARD vTemp;
	getAnGangedCard(vTemp);
	vAllCard.insert(vAllCard.end(), vTemp.begin(), vTemp.end());

	vTemp.clear();
	getMingGangedCard(vTemp);
	vAllCard.insert(vAllCard.end(), vTemp.begin(), vTemp.end());

	vTemp.clear();
	getPengedCard(vTemp);
	vAllCard.insert(vAllCard.end(), vTemp.begin(), vTemp.end());

	auto nType = card_Type(vAllCard.front());
	for (auto& ref : vAllCard)
	{
		auto tt = card_Type(ref);
		if (nType != tt)
		{
			return false;
		}
	}
	return true;
}

bool SDMJPlayerCard::checkDuiDuiHu() {
	VEC_CARD vAllCard;
	getHoldCard(vAllCard);
	uint8_t daCnt = daFilter(vAllCard);

	if (vAllCard.size() < 1) {
		return true;
	}

	std::sort(vAllCard.begin(), vAllCard.end());
	bool bFindJiang = false;
	uint8_t needDaCnt(0);
	for (uint8_t i = 0; i < vAllCard.size(); i++) {
		if (i + 2 < vAllCard.size() && vAllCard[i] == vAllCard[i + 2]) {
			i += 2;
			continue;
		}
		else if (i + 1 < vAllCard.size()) {
			if (vAllCard[i] == vAllCard[i + 1]) {
				if (bFindJiang) {
					needDaCnt++;
				}
				else {
					bFindJiang = true;
				}
				i++;
			}
			else {
				if (bFindJiang) {
					needDaCnt += 2;
				}
				else {
					bFindJiang = true;
					needDaCnt++;
				}
			}
		}
		else {
			if (bFindJiang) {
				needDaCnt += 2;
			}
			else {
				bFindJiang = true;
				needDaCnt++;
			}
		}
		if (needDaCnt > daCnt) {
			return false;
		}
	}

	if (!bFindJiang) {
		if (daCnt - needDaCnt > 1) {
			bFindJiang = true;
		}
	}

	return bFindJiang;
}

bool SDMJPlayerCard::checkQiDui() {
	return m_b7Pair;
}

bool SDMJPlayerCard::checkHaoHuaQiDui() {
	//update by haodi
	if (m_b7Pair == false) {
		return false;
	}

	if (isEnableHHQD() == false) {
		return false;
	}

	VEC_CARD vAllCard;
	getHoldCard(vAllCard);
	uint8_t eHunCnt = 0;
	if (!vecHu7Pair(vAllCard, eHunCnt)) {
		return false;
	}

	VEC_CARD v;
	getHoldCard(v);
	for (uint8_t nIdx = 0; (nIdx + 2) < v.size(); ++nIdx)
	{
		if (v[nIdx] == v[nIdx + 2])
		{
			return true;
		}
	}

	if (eHunCnt > 1) {
		return true;
	}

	return false;
}

uint8_t SDMJPlayerCard::checkAnKe(uint8_t nHuaCnt, bool checkDMQ, stHuDetail* pHuDetail) {
	auto sdHuDetail = dynamic_cast<stSDHuDetail*>(pHuDetail);
	//VEC_CARD vAllCard, vTemp, vFeng;
	//uint8_t daCnt;
	//每次验证清空
	m_vFengKe.clear();
	//getHoldCard(vAllCard);
	//if (vecHu7Pair(vAllCard, daCnt)) {
	//	return nHuaCnt;
	//}
	//daCnt = daFilter(vAllCard);
	if (m_b7Pair) {
		return nHuaCnt;
	}

	//分割牌中的普通牌和风牌
	/*std::sort(vAllCard.begin(), vAllCard.end());
	vTemp.clear();
	for (auto tCard : vAllCard) {
		auto tCardType = card_Type(tCard);
		if (tCardType == eCT_Feng) {
			vFeng.push_back(tCard);
		}
		else {
			vTemp.push_back(tCard);
		}
	}*/

	uint8_t nFengJiang = 0;
	uint8_t nFengCnts[4] = { 0,0,0,0 };
	for (uint8_t fengValue(1); fengValue < 5; fengValue++) {
		auto nFengCard = make_Card_Num(eCT_Feng, fengValue);
		uint8_t fengCount = std::count(m_vCards[eCT_Feng].begin(), m_vCards[eCT_Feng].end(), nFengCard);
		nFengCnts[fengValue - 1] = fengCount;
		if (fengCount > 2 && checkDMQ) {
			return nHuaCnt + 1;
		}
		if (fengCount && fengCount != 3) {
			if (nFengJiang && checkDMQ) {
				return nHuaCnt + 1;
			}
			else {
				if (nFengJiang == getHuCard()) {
					continue;
				}
			}
			nFengJiang = nFengCard;
		}
	}

	if (checkDMQ) {
		return nHuaCnt;
	}
	
	//修改为用更为取巧的方式去做,因为是关联使用,当发现不对时需要进行修改,谨慎修改代码
	if (card_Type(m_nJIang) != eCT_Feng) {
		nFengJiang = 0;
	}
	/*uint8_t nJiang = 0;
	if (nFengJiang && isHoldCardCanHuNormalNotFengJiang(nJiang)) {
		nFengJiang = 0;
	}*/

	//先计算手里的风的数量
	uint8_t needDaCnt = 0;
	for (uint8_t fengValue(0); fengValue < 4; fengValue++) {
		auto nFengCard = make_Card_Num(eCT_Feng, fengValue + 1);
		uint8_t fengCount = nFengCnts[fengValue];
		switch (fengCount) {
		case 1:
		case 2:
		{
			if (nFengJiang != nFengCard) {
				if (nFengCard == getHuCard())
				{
					if (sdHuDetail) {
						sdHuDetail->addMingKe();
					}
					nHuaCnt += 1;
					m_vFengKe.push_back(eFanXing_SD_FengMingKe);
				}
				else
				{
					if (sdHuDetail) {
						sdHuDetail->addAnKe();
					}
					nHuaCnt += 2;
					m_vFengKe.push_back(eFanXing_SD_FengAnKe);
				}
			}
		}
		break;
		case 3:
		{
			if (nFengCard == getHuCard())
			{
				if (sdHuDetail) {
					sdHuDetail->addMingKe();
				}
				nHuaCnt += 1;
				m_vFengKe.push_back(eFanXing_SD_FengMingKe);
			}
			else
			{
				if (sdHuDetail) {
					sdHuDetail->addAnKe();
				}
				nHuaCnt += 2;
				m_vFengKe.push_back(eFanXing_SD_FengAnKe);
			}
		}
		break;
		case 4: {
			if (nFengJiang == nFengCard) {
				if (nFengCard == getHuCard())
				{
					if (sdHuDetail) {
						sdHuDetail->addMingKe();
					}
					nHuaCnt += 1;
					m_vFengKe.push_back(eFanXing_SD_FengMingKe);
				}
				else
				{
					if (sdHuDetail) {
						sdHuDetail->addAnKe();
					}
					nHuaCnt += 2;
					m_vFengKe.push_back(eFanXing_SD_FengAnKe);
				}
			}
			else {
				if (nFengCard == getHuCard()) {
					if (sdHuDetail) {
						sdHuDetail->addMingKe();
					}
					if (sdHuDetail) {
						sdHuDetail->addAnKe();
					}
					nHuaCnt += 3;
					m_vFengKe.push_back(eFanXing_SD_FengMingKe);
					m_vFengKe.push_back(eFanXing_SD_FengAnKe);
				}
				else {
					if (sdHuDetail) {
						sdHuDetail->addAnKe(2);
					}
					nHuaCnt += 4;
					m_vFengKe.push_back(eFanXing_SD_FengAnKe);
					m_vFengKe.push_back(eFanXing_SD_FengAnKe);
				}
			}
		}
		break;
		}
	}
	return nHuaCnt;
}

void SDMJPlayerCard::getAnKeHuaType(std::vector<eFanxingType> &vFanxing) {
	for (auto ref : m_vFengKe) {
		vFanxing.push_back((eFanxingType)ref);
	}
}

void SDMJPlayerCard::setRuleMode(uint8_t nRuleMode) {
	if (nRuleMode == 1 || nRuleMode == 2) {
		m_nRuleMode == nRuleMode;
	}
}

uint8_t SDMJPlayerCard::getZiMoHuaRequire()
{
	if (1 == m_nRuleMode)
	{
		return 2;
	}
	else if (2 == m_nRuleMode)
	{
		return 3;
	}
	return 3;
}

uint8_t SDMJPlayerCard::getDianPaoHuHuaRequire()
{
	if (1 == m_nRuleMode)
	{
		return 3;
	}
	else if (2 == m_nRuleMode)
	{
		return 4;
	}
	return 4;
}

uint8_t SDMJPlayerCard::isHaveDa() {
	auto daCardType = card_Type(getBaiDaCard());
	if (daCardType > eCT_Hua)
	{
		LOGFMTE("isHaveDa parse card type error so do not have this card = %u", getBaiDaCard());
		return 0;
	}
	auto& vCard = m_vCards[daCardType];
	auto cnt = std::count(vCard.begin(), vCard.end(), getBaiDaCard());
	return cnt;
}

bool SDMJPlayerCard::checkRaoDa(bool isZiMo, uint8_t nCard) {
	auto nBaiDaCnt = isHaveDa();

	if (nBaiDaCnt == 0) {
		return false;
	}

	//TODO 7Pair RaoDa?!

	VEC_CARD backCards[eCT_Max];
	for (uint8_t nType = eCT_None; nType < eCT_Max; nType++) {
		backCards[nType].insert(backCards[nType].begin(), m_vCards[nType].begin(), m_vCards[nType].end());
	}

	bool bRaoDa = false;
	do {
		if (isZiMo) {
			if (getNewestFetchedCard() == getBaiDaCard()) {
				if (nBaiDaCnt < 2) {
					break;
				}
				nBaiDaCnt -= 2;
			}
			else {
				removeHoldCard(getNewestFetchedCard());
				nBaiDaCnt--;
			}
		}
		else {
			nBaiDaCnt--;
		}
		removeHoldCardAllBaiDa();

		bRaoDa = check7PairRaoDaAllShun(nBaiDaCnt) || CheckHoldCardAllShun(nBaiDaCnt);
	} while (false);

	for (uint8_t nType = eCT_None; nType < eCT_Max; nType++) {
		m_vCards[nType].clear();
		m_vCards[nType].insert(m_vCards[nType].begin(), backCards[nType].begin(), backCards[nType].end());
	}

	return bRaoDa;
}

bool SDMJPlayerCard::check7PairRaoDaAllShun(uint8_t nBaiDaCnt) {
	if (isEanble7Pair() == false) {
		return false;
	}

	VEC_CARD vHold;
	getHoldCard(vHold);
	if ((vHold.size() + nBaiDaCnt) < 12)
	{
		return false;
	}
	std::sort(vHold.begin(), vHold.end());
	for (size_t nIdx = 0; (nIdx + 1) < vHold.size(); )
	{
		if (vHold[nIdx] != vHold[nIdx + 1])
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
	return true;
}

bool SDMJPlayerCard::vecHu7Pair(VEC_CARD vHuPai, uint8_t& eHunCnt) {
	if (vHuPai.size() != 14)
	{
		return false;
	}
	std::sort(vHuPai.begin(), vHuPai.end());
	eHunCnt = daFilter(vHuPai);
	for (size_t nIdx = 0; (nIdx + 1) < vHuPai.size(); )
	{
		if (vHuPai[nIdx] != vHuPai[nIdx + 1])
		{
			if (eHunCnt >= 1)
			{
				++nIdx;
				--eHunCnt;
				continue;
			}
			return false;
		}
		nIdx += 2;
	}
	return true;
}

bool SDMJPlayerCard::isEanble7Pair() {
	return ((SDMJRoom*)(getPlayer()->getRoom()))->isEnableQiDui();
}

bool SDMJPlayerCard::isEnableHHQD() {
	return ((SDMJRoom*)(getPlayer()->getRoom()))->isEanableHHQD();
}

void SDMJPlayerCard::removeHoldCardAllBaiDa() {
	while (isHaveDa()) {
		removeHoldCard(getBaiDaCard());
	}
}

uint8_t SDMJPlayerCard::daFilter(VEC_CARD& nCards) {
	uint8_t daCnt(0);
	uint8_t i = 0;
	while (i < 14) {
		VEC_CARD::iterator it = std::find(nCards.begin(), nCards.end(), getBaiDaCard());
		if (it == nCards.end()) {
			break;
		}
		else {
			daCnt++;
			nCards.erase(it);
		}
		i++;
	}
	return daCnt;
}

uint8_t SDMJPlayerCard::getSDMiniQueCntMustJiang(VEC_CARD tCards, uint8_t nDaCnt) {
	VEC_CARD vCards[eCT_Max];
	for (auto nCard : tCards)
	{
		auto eType = card_Type(nCard);
		if (eType > eCT_None || eType < eCT_Max)
		{
			vCards[eType].push_back(nCard);
		}
	}

	VEC_CARD vNotShun;
	uint8_t nAlreadyQueCnt = 0;
	for (uint8_t nType = eCT_None; nType < eCT_Max; ++nType)
	{
		uint8_t nQueCnt = 0;
		tryToFindMiniQueCnt(vCards[nType], isCardTypeMustKeZi(nType), vNotShun, nQueCnt, nDaCnt - nAlreadyQueCnt);
		nAlreadyQueCnt += nQueCnt;
		if (nAlreadyQueCnt > nDaCnt)
		{
			return 100;
		}
	}
	return nAlreadyQueCnt;
}

bool SDMJPlayerCard::isHoldCardCanHuNormalNotFengJiang(uint8_t& nJiang) {
	auto nBaiDaCnt = isHaveDa();

	// remove bai da card from hold 
	removeHoldCardAllBaiDa();

	// check hu
	bool isHu = false;
	bool isBreak = false;

	do
	{
		VEC_CARD vHoldCards;
		getHoldCard(vHoldCards);

		std::set<uint8_t> vMayBeJiang;
		for (uint8_t nIdx = 0; (nIdx + 1) < vHoldCards.size(); )
		{
			if (vHoldCards[nIdx] == vHoldCards[nIdx + 1])
			{
				if (card_Type(vHoldCards[nIdx]) != eCT_Feng) {
					vMayBeJiang.insert(vHoldCards[nIdx]);
					nIdx += 2;
					continue;
				}
			}
			++nIdx;
		}

		for (auto& ref : vMayBeJiang)
		{
			// remove jiang ;
			removeHoldCard(ref);
			removeHoldCard(ref);
			// check all card type is shun, without jiang ;
			isHu = CheckHoldCardAllShun(nBaiDaCnt);
			// add back jiang 
			addHoldCard(ref);
			addHoldCard(ref);

			if (isHu)  // ok this jiang can hu , so need not go on try other jiang 
			{
				nJiang = ref;
				isBreak = true;
				break;
			}
		}

		if (isBreak) {
			break;
		}

		// one baiDa as Jiang 
		if (nBaiDaCnt >= 1)
		{
			vMayBeJiang.clear();
			vMayBeJiang.insert(vHoldCards.begin(), vHoldCards.end());  // erase duplicate cards ;
			for (auto& ref : vMayBeJiang)
			{
				if (card_Type(ref) == eCT_Feng) {
					continue;
				}
				// remove jiang ;
				removeHoldCard(ref);
				// check all card type is shun, without jiang ;
				isHu = CheckHoldCardAllShun(nBaiDaCnt - 1);
				// add back jiang 
				addHoldCard(ref);

				if (isHu)  // ok this jiang can hu , so need not go on try other jiang 
				{
					nJiang = ref;
					isBreak = true;
					break;
				}
			}
		}

		if (isBreak) {
			break;
		}

		// 2 baiDa as jiang 
		if (nBaiDaCnt >= 2)
		{
			if (CheckHoldCardAllShun(nBaiDaCnt - 2))
			{
				nJiang = 0; // zero means baiDa as jiang ;
				isHu = true;
				isBreak = true;
				break;
			}
		}

		if (isBreak) {
			break;
		}
	} while (false);

	// add back baiDa to hold 
	while (nBaiDaCnt--)
	{
		addHoldCard(getBaiDaCard());
	}
	return isHu;
}