#include "NJMJPlayerCard.h"
#include "IMJPoker.h"
#include "log4z.h"
#include "FanxingHunYiSe.h"
#include "FanxingQingYiSe.h"
#include "FanxingDuiDuiHu.h"
#include "Fanxing7Dui.h"
#include "NJMJPlayer.h"
#include "NJMJRoom.h"

void NJMJPlayerCard::reset() {
	MJPlayerCard::reset();
	m_nHuCard = 0;
}

//bool NJMJPlayerCard::canHuWithCard(uint8_t nCard, bool bNormal) {
//	if (bNormal) {
//		return MJPlayerCard::canHuWitCard(nCard);
//	}
//	else {
//		return canHuWitCard(nCard);
//	}
//}

bool NJMJPlayerCard::canHuWitCard(uint8_t nCard) {
	auto eType = card_Type(nCard);
	if (eType >= eCT_Max)
	{
		LOGFMTE("parse card type error ,canHuWitCard have this card = %u", nCard);
		return false;
	}

	uint8_t nBaoIdx = -1;
	auto pPlayer = (NJMJPlayer*)getPlayer();
	auto pRoom = (NJMJRoom*)pPlayer->getRoom();
	if (isKuaiZhaoHu(nCard, nBaoIdx)) {
		if (pRoom->checkBaoGuang(nBaoIdx)) {
			pRoom->onPlayerLouHu(pPlayer->getIdx(), nBaoIdx);
			return false;
		}
		return true;
	}

	addHoldCard(nCard);

	uint8_t nJiang = 0;
	bool bSelfHu = MJPlayerCard::isHoldCardCanHu(nJiang);
	if (bSelfHu) {
		do
		{
			if (getWaiBaoIdx(nBaoIdx)) {
				if (pRoom->checkBaoGuang(nBaoIdx)) {
					bSelfHu = false;
					break;
				}
			}
			else if (getNormalBaoIdx(nBaoIdx)) {
				if (pRoom->checkGuang(nBaoIdx)) {
					bSelfHu = false;
					break;
				}
			}
			else if (pRoom->checkHuGuang(nBaoIdx)) {
				bSelfHu = false;
				break;
			}

			auto nBuHuaCnt = getBuHuaCnt();
			if (nBuHuaCnt < 4) {
				bSelfHu = checkDaHu();
			}
			
		} while (false);

		if (bSelfHu) {
			bSelfHu = checkYiDuiDaoDi();
		}
		else {
			pRoom->onPlayerLouHu(pPlayer->getIdx(), nBaoIdx);
		}
	}

	removeHoldCard(nCard);
	//debugCardInfo();
	return bSelfHu;
}

void NJMJPlayerCard::onDoHu(uint16_t nInvokerIdx, uint8_t nHuCard, bool isInvokerHaveGangFlag) {
	/*m_nDianPaoIdx = nInvokerIdx;
	onMoCard(nHuCard);
	uint8_t nJiang = 0;
	if (!MJPlayerCard::isHoldCardCanHu(nJiang))
	{
		return;
		LOGFMTE("you can not hu , why do hu ?");
	}
	m_nJIang = nJiang;*/
	//TODO
	return;
}

bool NJMJPlayerCard::isHoldCardCanHu(uint8_t& nJiang) {
	//TODO
	uint8_t nBaoIdx = -1;
	auto pPlayer = (NJMJPlayer*)getPlayer();
	auto pRoom = (NJMJRoom*)pPlayer->getRoom();
	if (isKuaiZhaoZiMo(nBaoIdx)) {
		if (pRoom->checkBaoGuang(nBaoIdx)) {
			return false;
		}
		return true;
	}

	bool bSelfHu = MJPlayerCard::isHoldCardCanHu(nJiang);

	if (bSelfHu) {
		do
		{
			if (getWaiBaoIdx(nBaoIdx, true)) {
				if (pRoom->checkBaoGuang(nBaoIdx)) {
					bSelfHu = false;
					break;
				}
			}
			else if (getNormalBaoIdx(nBaoIdx, true)) {
				if (pRoom->checkGuang(nBaoIdx)) {
					bSelfHu = false;
					break;
				}
			}

			auto nBuHuaCnt = getBuHuaCnt();
			if (nBuHuaCnt < 4) {
				bSelfHu = checkDaHu();
			}

		} while (false);

		if (bSelfHu) {
			bSelfHu = checkYiDuiDaoDi();
		}
	}

	return bSelfHu;
}

bool NJMJPlayerCard::isHaveCards(VEC_CARD vCards) {
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

bool NJMJPlayerCard::eraseVector(uint8_t p, VEC_CARD& typeVec)
{
	VEC_CARD::iterator it = find(typeVec.begin(), typeVec.end(), p);
	if (it != typeVec.end())
	{
		typeVec.erase(it);
		return true;
	}
	return false;
}

uint8_t NJMJPlayerCard::getHoldCardCnt() {
	VEC_CARD vHoldCard;
	getHoldCard(vHoldCard);
	return vHoldCard.size();
}

//uint8_t NJMJPlayerCard::getHuaCard() {
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

uint8_t NJMJPlayerCard::getHuaCntWithoutHuTypeHuaCnt() {
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

bool NJMJPlayerCard::checkHunYiSe() {
	FanxingHunYiSe checker;
	return checker.checkFanxing(this, nullptr, 0, nullptr);
}

bool NJMJPlayerCard::checkQingYiSe() {
	FanxingQingYiSe checker;
	return checker.checkFanxing(this, nullptr, 0, nullptr);
}

bool NJMJPlayerCard::checkDuiDuiHu() {
	FanxingDuiDuiHu checker;
	return checker.checkFanxing(this, nullptr, 0, nullptr);
}

bool NJMJPlayerCard::checkQiDui() {
	Fanxing7Dui checker;
	return checker.checkFanxing(this, nullptr, 0, nullptr);
}

uint8_t NJMJPlayerCard::checkAnKe(uint8_t nHuaCnt) {
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

bool NJMJPlayerCard::isSiLianFeng() {
	if (m_vChuedCard.size() == 4) {
		uint8_t nValue = 1;
		while (nValue < 5)
		{
			auto iter = std::find(m_vChuedCard.begin(), m_vChuedCard.end(), make_Card_Num(eCT_Feng, nValue));
			if (iter == m_vChuedCard.end()) {
				return false;
			}
			nValue++;
		}
		return true;
	}
	return false;
}

bool NJMJPlayerCard::isZiDaAnGang(uint8_t nCard) {
	return std::count(m_vChuedCard.begin(), m_vChuedCard.end(), nCard) == 4;
}

bool NJMJPlayerCard::checkDaHu() {
	if (checkTianHu() || checkDiHu()) {
		return true;
	}

	if (checkQiDui()) {
		return true;
	}

	if (checkDuiDuiHu() || checkHunYiSe() || checkQingYiSe()) {
		return true;
	}

	if (checkMenQing()) {
		return true;
	}

	if (checkWuHuaGuo()) {
		return true;
	}

	return false;
}

bool NJMJPlayerCard::checkYiDuiDaoDi() {
	auto pRoom = (NJMJRoom*)getPlayer()->getRoom();
	if (pRoom->isEnableYiDuiDaoDi()) {
		VEC_CARD vPenged;
		getPengedCard(vPenged);
		if (vPenged.size()) {
			return checkQingYiSe() || checkHunYiSe() || checkDuiDuiHu();
		}
	}
	return true;
}