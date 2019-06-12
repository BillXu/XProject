#include "SZMJPlayerCard.h"
#include "IMJPoker.h"
#include "log4z.h"
#include "FanxingHunYiSe.h"
#include "FanxingQingYiSe.h"
#include "FanxingDuiDuiHu.h"
#include "Fanxing7Dui.h"

void SZMJPlayerCard::reset() {
	MJPlayerCard::reset();
	m_nHuCard = 0;
}

bool SZMJPlayerCard::canHuWitCard(uint8_t nCard) {
	return canHuWitCard(nCard, false);
}

bool SZMJPlayerCard::canHuWitCard(uint8_t nCard, bool isRotGang) {
	auto eType = card_Type(nCard);
	if (eType >= eCT_Max)
	{
		LOGFMTE("parse card type error ,canHuWitCard have this card = %u", nCard);
		return false;
	}

	addHoldCard(nCard);
	setHuCard(nCard);
	uint8_t nJiang = 0;
	bool bSelfHu = MJPlayerCard::isHoldCardCanHu(nJiang);
	if (bSelfHu) {
		auto nHuaCnt = getHuaCntWithoutHuTypeHuaCnt();
		if (nHuaCnt < getDianPaoHuHuaRequire()) // not enought hua cnt ;
		{
			auto isDaHu = checkDaMenQing() || checkXiaoMenQing() || checkHunYiSe() || checkQingYiSe() || checkDuiDuiHu() || checkQiDui();
			if (isDaHu == false)  // not da hua , then xiao hu need hua cnt ; so can not hu this card ;
			{
				if (nHuaCnt + 1 == getDianPaoHuHuaRequire() && isRotGang) {

				}
				else {
					bSelfHu = false;
				}
			}
		}
	}
	removeHoldCard(nCard);
	setHuCard(0);
	//debugCardInfo();
	return bSelfHu;
}

void SZMJPlayerCard::onDoHu(uint16_t nInvokerIdx, uint8_t nHuCard, bool isInvokerHaveGangFlag) {
	m_nDianPaoIdx = nInvokerIdx;
	onMoCard(nHuCard);
	uint8_t nJiang = 0;
	if (!MJPlayerCard::isHoldCardCanHu(nJiang))
	{
		return;
		LOGFMTE("you can not hu , why do hu ?");
	}
	m_nJIang = nJiang;
	return;
}

bool SZMJPlayerCard::isHoldCardCanHu(uint8_t& nJiang) {
	//TODO
	return isHoldCardCanHu(nJiang, false);
}

bool SZMJPlayerCard::isHoldCardCanHu(uint8_t& nJiang, bool isGangKai) {
	bool bSelfHu = MJPlayerCard::isHoldCardCanHu(nJiang);
	if (bSelfHu)
	{
		auto nHuaCnt = getHuaCntWithoutHuTypeHuaCnt();
		if (nHuaCnt < getZiMoHuaRequire()) // not enought hua cnt ;
		{
			auto isDaHu = isGangKai || checkDaMenQing() || checkXiaoMenQing() || checkHunYiSe() || checkQingYiSe() || checkDuiDuiHu() || checkQiDui();
			if (isDaHu == false)  // not da hu , then xiao hu need hua cnt ; so can not hu this card ;
			{
				bSelfHu = false;
			}
		}
	}
	//debugCardInfo();
	return bSelfHu;
}

bool SZMJPlayerCard::isHaveCards(VEC_CARD vCards) {
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

bool SZMJPlayerCard::eraseVector(uint8_t p, VEC_CARD& typeVec)
{
	VEC_CARD::iterator it = find(typeVec.begin(), typeVec.end(), p);
	if (it != typeVec.end())
	{
		typeVec.erase(it);
		return true;
	}
	return false;
}

uint8_t SZMJPlayerCard::getHoldCardCnt() {
	VEC_CARD vHoldCard;
	getHoldCard(vHoldCard);
	return vHoldCard.size();
}

//uint8_t SZMJPlayerCard::getHuaCard() {
//	if (m_vCards[eCT_Jian].size()) {
//		return m_vCards[eCT_Jian].front();
//	}
//	else if (m_vCards[eCT_Hua].size()) {
//		for (auto tCard : m_vCards[eCT_Hua]) {
//			if (9 != card_Value(tCard)) {
//				return tCard;
//			}
//		}
//	}
//	return 0;
//}

uint8_t SZMJPlayerCard::getHuaCntWithoutHuTypeHuaCnt() {
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
	// check feng an ke 
	//update by haodi
	nHuaCnt = checkAnKe(nHuaCnt);

	return nHuaCnt;
}

bool SZMJPlayerCard::checkDaMenQing() {
	return getHuaCntWithoutHuTypeHuaCnt() == 0;
}

bool SZMJPlayerCard::checkXiaoMenQing() {
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

bool SZMJPlayerCard::checkHunYiSe() {
	FanxingHunYiSe checker;
	return checker.checkFanxing(this, nullptr, 0, nullptr);
}

bool SZMJPlayerCard::checkQingYiSe() {
	FanxingQingYiSe checker;
	return checker.checkFanxing(this, nullptr, 0, nullptr);
}

bool SZMJPlayerCard::checkDuiDuiHu() {
	FanxingDuiDuiHu checker;
	return checker.checkFanxing(this, nullptr, 0, nullptr);
}

bool SZMJPlayerCard::checkQiDui() {
	Fanxing7Dui checker;
	return checker.checkFanxing(this, nullptr, 0, nullptr);
}

uint8_t SZMJPlayerCard::checkAnKe(uint8_t nHuaCnt) {
	auto& vFeng = m_vCards[eCT_Feng];
	for (uint8_t nidx = 0; (nidx + 2) < vFeng.size(); )
	{
		if (vFeng[nidx] == vFeng[nidx + 2])
		{
			if (nidx + 3 < vFeng.size()) {
				if (vFeng[nidx] != vFeng[nidx + 3]) {
					if (vFeng[nidx] == getHuCard())
					{
						nHuaCnt += 1;
					}
					else
					{
						nHuaCnt += 2;
					}
					nidx += 3;
				}
				else {
					nidx += 4;
				}
			}
			else {
				if (vFeng[nidx] == getHuCard())
				{
					nHuaCnt += 1;
				}
				else
				{
					nHuaCnt += 2;
				}
				nidx += 3;
			}
			//nidx += 3;
			//continue;
		}
		else {
			++nidx;
		}
	}
	return nHuaCnt;
}

void SZMJPlayerCard::setRuleMode(uint8_t nRuleMode) {
	if (nRuleMode == 1 || nRuleMode == 2) {
		m_nRuleMode == nRuleMode;
	}
}

uint8_t SZMJPlayerCard::getZiMoHuaRequire()
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

uint8_t SZMJPlayerCard::getDianPaoHuHuaRequire()
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