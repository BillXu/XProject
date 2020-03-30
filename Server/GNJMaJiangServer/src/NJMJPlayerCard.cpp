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
	m_nQQDDCard = 0;
	clearTing();
	clearQQDD();
	clearKuaiZhao();
}

//bool NJMJPlayerCard::canHuWithCard(uint8_t nCard, bool bNormal) {
//	if (bNormal) {
//		return MJPlayerCard::canHuWitCard(nCard);
//	}
//	else {
//		return canHuWitCard(nCard);
//	}
//}

bool NJMJPlayerCard::canPengWithCard(uint8_t nCard) {
	if (isTing()) {
		return false;
	}
	return MJPlayerCard::canPengWithCard(nCard);
}

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
	setHuCard(nCard);

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
	clearHuCard();
	//debugCardInfo();
	return bSelfHu;
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

void NJMJPlayerCard::onDoHu(uint16_t nInvokerIdx, uint8_t nHuCard, bool isInvokerHaveGangFlag) {
	m_nDianPaoIdx = nInvokerIdx;
	onMoCard(nHuCard);
	setHuCard(nHuCard);
	/*uint8_t nJiang = 0;
	if (!MJPlayerCard::isHoldCardCanHu(nJiang))
	{
		return;
		LOGFMTE("you can not hu , why do hu ?");
	}
	m_nJIang = nJiang;*/
	//TODO
	return;
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

bool NJMJPlayerCard::isPengCard(uint8_t nCard) {
	for (auto ref_ming : m_vMingCardInfo) {
		if (ref_ming.eAct == eMJAct_Peng && ref_ming.nTargetCard == nCard) {
			return true;
		}
	}

	return false;
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

uint8_t NJMJPlayerCard::getHuaCntWithoutHuTypeHuaCnt(std::vector<eFanxingType>& vFanxing, bool isKuaiZhaoHu) {
	VEC_CARD vCard, vHuaCard;
	getBuHuaCard(vHuaCard);
	uint8_t nHuaCnt = vHuaCard.size();
	// check ming gang 
	getMingGangedCard(vCard);
	for (auto& ref : vCard)
	{
		if (card_Type(ref) == eCT_Feng)
		{
			nHuaCnt += 2;
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
			nHuaCnt += 3;
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
	checkAnKe(nHuaCnt);

	if (isKuaiZhaoHu == false) {
		auto iterYajue = std::find(vFanxing.begin(), vFanxing.end(), eFanxing_YaJue);
		if (iterYajue == vFanxing.end()) {
			if (isBianHu()) {
				nHuaCnt++;
				vFanxing.push_back(eFanxing_BianHu);
			}
			else if (isJiaHu()) {
				nHuaCnt++;
				vFanxing.push_back(eFanxing_JiaHu);
			}
			else if (isDanDiao()) {
				nHuaCnt++;
				vFanxing.push_back(eFanxing_DanDiao);
			}
		}
	}

	if (isQueYi()) {
		nHuaCnt++;
		vFanxing.push_back(eFanxing_QueYi);
	}

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
	return m_b7Pair;
}

uint8_t NJMJPlayerCard::checkAnKe(uint8_t& nHuaCnt) {
	auto& vFeng = m_vCards[eCT_Feng];
	for (uint8_t nidx = 0; (nidx + 2) < vFeng.size(); )
	{
		if (vFeng[nidx] == vFeng[nidx + 2])
		{
			if (nidx + 3 < vFeng.size()) {
				if (vFeng[nidx] != vFeng[nidx + 3]) {
					/*if (vFeng[nidx] == getHuCard())
					{
						nHuaCnt += 1;
					}
					else
					{
						nHuaCnt += 2;
					}*/
					nHuaCnt += 1;
					nidx += 3;
				}
				else {
					nHuaCnt += 3;
					nidx += 4;
				}
			}
			else {
				/*if (vFeng[nidx] == getHuCard())
				{
					nHuaCnt += 1;
				}
				else
				{
					nHuaCnt += 2;
				}*/
				nHuaCnt += 1;
				nidx += 3;
			}
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

void NJMJPlayerCard::getNormalHuType(std::vector<eFanxingType>& vFanxing, bool isZiMo) {
	if (checkTianHu()) {
		vFanxing.push_back(eFanxing_TianHu);
	}
	else if (checkDiHu()) {
		vFanxing.push_back(eFanxing_DiHu);
	}

	if (checkQiDui(vFanxing, isZiMo) == false) {
		if (checkDuiDuiHu()) {
			vFanxing.push_back(eFanxing_DuiDuiHu);
		}

		if (checkQuanQiuDuDiao()) {
			vFanxing.push_back(eFanxing_QuanQiuDuDiao);
		}
	}

	if (checkQingYiSe()) {
		vFanxing.push_back(eFanxing_QingYiSe);
	}
	else if (checkHunYiSe()) {
		vFanxing.push_back(eFanxing_HunYiSe);
	}

	if (checkMenQing()) {
		vFanxing.push_back(eFanxing_MengQing);
	}

	if (checkYaJue()) {
		vFanxing.push_back(eFanxing_YaJue);
	}

	if (checkWuHuaGuo(vFanxing.size())) {
		vFanxing.push_back(eFanxing_WuHuaGuo);
	}

	if (vFanxing.size() == 0 ||
		(vFanxing.size() == 1 && vFanxing[0] == eFanxing_MengQing)) {
		vFanxing.push_back(eFanxing_PingHu);
	}
}

bool NJMJPlayerCard::onChuCard(uint8_t nChuCard) {
	if (MJPlayerCard::onChuCard(nChuCard)) {
		VEC_CARD vHoldCard;
		getHoldCard(vHoldCard);
		if (vHoldCard.size() == 1) {
			auto nCard = vHoldCard[0];
			if (isQQDD()) {
				if (nCard == m_nQQDDCard) {
					//TODO NOTHING
				}
				else {
					clearQQDD();
				}
			}
			else {
				if (m_nQQDDCard) {
					//TODO NOTHING
				}
				else {
					auto nType = card_Type(nCard);
					auto nChuType = card_Type(nChuCard);
					if (nType == nChuType) {
						if (nType == eCT_Feng) {
							signQQDD();
							m_nQQDDCard = nCard;
						}
						else {
							auto nValue = card_Value(nCard);
							auto nChuValue = card_Value(nChuCard);
							uint8_t nMinValue = nChuValue < 4 ? 1 : nChuValue - 2;
							uint8_t nMaxValue = nChuValue > 6 ? 9 : nChuValue + 2;
							if (nValue < nMinValue || nValue > nMaxValue) {
								//TODO NOTHING
							}
							else {
								signQQDD();
								m_nQQDDCard = nCard;
							}
						}
					}
				}
			}
		}
		return true;
	}
	return false;
}

bool NJMJPlayerCard::isKuaiZhaoHu(uint8_t nCard, uint8_t& nBaoIdx) {
	m_bKuaiZhaoHu = false;
	VEC_CARD vHoldCard, vCheckCard;
	getHoldCard(vHoldCard);
	if (vHoldCard.size() == 4) {
		vCheckCard.clear();
		vCheckCard.push_back(nCard);
		vCheckCard.push_back(nCard);
		if (isHaveCards(vCheckCard)) {
			vCheckCard.clear();
			uint8_t nTempBao = -1;
			uint8_t nPengTimes = 0;
			uint8_t nCurPengTimes = 0;
			for (auto ref_ming : m_vMingCardInfo) {
				if(ref_ming.eAct == eMJAct_Peng || ref_ming.eAct == eMJAct_MingGang || ref_ming.eAct == eMJAct_BuGang) {
					vCheckCard.push_back(ref_ming.nTargetCard);
					nPengTimes++;
					if (nTempBao == -1) {
						nTempBao = ref_ming.nInvokerIdx;
						nCurPengTimes++;
					}
					else {
						if (nTempBao == ref_ming.nInvokerIdx) {
							nCurPengTimes++;
						}
						else {
							nTempBao == -1;
							nCurPengTimes = 0;
						}
					}
				}
				else if (ref_ming.eAct == eMJAct_AnGang) {
					vCheckCard.push_back(ref_ming.nTargetCard);
					nPengTimes++;
					nCurPengTimes++;
				}
			}
			if (nPengTimes == 3) {
				if (nCurPengTimes == 3) {
					m_bKuaiZhaoHu = true;
					nBaoIdx = nTempBao;
					return true;
				}

				auto nType = card_Type(nCard);
				for (auto ref_check : vCheckCard) {
					auto nRefType = card_Type(ref_check);
					if (nRefType != nType) {
						return false;
					}
				}
				m_bKuaiZhaoHu = true;
				return true;
			}
		}
	}

	return false;
}

bool NJMJPlayerCard::isKuaiZhaoZiMo(uint8_t& nBaoIdx) {
	m_bKuaiZhaoZiMo = false;
	VEC_CARD vHoldCard, vCheckCard;
	getHoldCard(vHoldCard);
	if (vHoldCard.size() == 5) {
		uint8_t nCard = getNewestFetchedCard();
		vCheckCard.push_back(nCard);
		vCheckCard.push_back(nCard);
		vCheckCard.push_back(nCard);
		if (isHaveCards(vCheckCard)) {
			vCheckCard.clear();
			uint8_t nTempBao = -1;
			uint8_t nPengTimes = 0;
			uint8_t nCurPengTimes = 0;
			for (auto ref_ming : m_vMingCardInfo) {
				if (ref_ming.eAct == eMJAct_Peng || ref_ming.eAct == eMJAct_MingGang || ref_ming.eAct == eMJAct_BuGang) {
					vCheckCard.push_back(ref_ming.nTargetCard);
					nPengTimes++;
					if (nTempBao == -1) {
						nTempBao = ref_ming.nInvokerIdx;
						nCurPengTimes++;
					}
					else {
						if (nTempBao == ref_ming.nInvokerIdx) {
							nCurPengTimes++;
						}
						else {
							nTempBao == -1;
							nCurPengTimes = 0;
						}
					}
				}
				else if (ref_ming.eAct == eMJAct_AnGang) {
					vCheckCard.push_back(ref_ming.nTargetCard);
					nPengTimes++;
					nCurPengTimes++;
				}
			}
			if (nPengTimes == 3 && nCurPengTimes == 3 && nTempBao != -1) {
				m_bKuaiZhaoZiMo = true;
				nBaoIdx = nTempBao;
				return true;
			}
		}
	}

	return false;
}

bool NJMJPlayerCard::checkMenQing() {
	for (auto ref_ming : m_vMingCardInfo) {
		if (ref_ming.eAct == eMJAct_Peng) {
			return false;
		}
	}
	return true;
}

bool NJMJPlayerCard::checkWuHuaGuo(bool isDaHu) {
	if (isDaHu) {
		for (auto ref_ming : m_vMingCardInfo) {
			if (ref_ming.eAct == eMJAct_BuHua) {
				return false;
			}
		}
		return true;
	}

	return false;
}

bool NJMJPlayerCard::checkTianHu() {
	auto pPlayer = (NJMJPlayer*)getPlayer();
	return pPlayer->haveFlag(IMJPlayer::eMJActFlag_CanTianHu);
}

bool NJMJPlayerCard::checkDiHu() {
	return isTing();
}

bool NJMJPlayerCard::checkYaJue() {
	if (isBianHu() || isJiaHu()) {
		auto pRoom = (NJMJRoom*)getPlayer()->getRoom();
		return pRoom->isAnyPlayerPengCard(getHuCard());
	}

	return false;
}

bool NJMJPlayerCard::checkQuanQiuDuDiao() {
	return getHoldCardCnt() == 2;
}

bool NJMJPlayerCard::isJiaHu() {
	if (getHoldCardCnt() < 5) {
		return false;
	}
	auto nCard = getHuCard();
	if (isHaveCard(nCard) == false) {
		return false;
	}
	auto nType = card_Type(nCard);
	if (nType != eCT_Tiao && nType != eCT_Tong && nType != eCT_Wan) {
		return false;
	}
	auto nValue = card_Value(nCard);
	if (nValue == 1 || nValue == 9) {
		return false;
	}
	auto vCards = m_vCards[nType];
	if (vCards.size() < 3) {
		return false;
	}
	uint8_t nCnt = std::count_if(vCards.begin(), vCards.end(), [nCard](uint8_t& tCard) {
		return nCard == tCard;
	});
	auto nCardPre = make_Card_Num(nType, nValue - 1);
	uint8_t nCntPre = std::count_if(vCards.begin(), vCards.end(), [nCardPre](uint8_t& tCard) {
		return nCardPre == tCard;
	});
	if (nCntPre < 1) {
		return false;
	}
	auto nCardAfter = make_Card_Num(nType, nValue + 1);
	uint8_t nCntAfter = std::count_if(vCards.begin(), vCards.end(), [nCardAfter](uint8_t& tCard) {
		return nCardAfter == tCard;
	});
	if (nCntAfter < 1) {
		return false;
	}

	bool bFlag = false;
	auto nJiang = m_nJIang;
	removeHoldCard(nCard);
	removeHoldCard(nCardPre);
	removeHoldCard(nCardAfter);
	bFlag = MJPlayerCard::isHoldCardCanHu(m_nJIang);
	addHoldCard(nCard);
	addHoldCard(nCardPre);
	addHoldCard(nCardAfter);
	m_nJIang = nJiang;
	return bFlag;
}

bool NJMJPlayerCard::isDanDiao() {
	if (getHoldCardCnt() < 3) {
		return true;
	}
	auto nCard = getHuCard();
	if (isHaveCard(nCard) == false) {
		return false;
	}
	auto nType = card_Type(nCard);
	auto vCards = m_vCards[nType];

	uint8_t nCnt = std::count_if(vCards.begin(), vCards.end(), [nCard](uint8_t& tCard) {
		return nCard == tCard;
	});
	if (nCnt < 2) {
		return false;
	}
	if (m_nJIang == nCard) {
		return true;
	}
	bool bFlag = false;
	removeHoldCard(nCard);
	removeHoldCard(nCard);
	bFlag = CheckHoldCardAllShun();
	addHoldCard(nCard);
	addHoldCard(nCard);
	return bFlag;
}

bool NJMJPlayerCard::isBianHu() {
	if (getHoldCardCnt() < 5) {
		return false;
	}
	auto nCard = getHuCard();
	if (isHaveCard(nCard) == false) {
		return false;
	}
	auto nType = card_Type(nCard);
	if (nType != eCT_Tiao && nType != eCT_Tong && nType != eCT_Wan) {
		return false;
	}
	auto nValue = card_Value(nCard);
	if (nValue != 7 && nValue != 3) {
		return false;
	}
	auto vCards = m_vCards[nType];
	if (vCards.size() < 3) {
		return false;
	}
	uint8_t nCnt = std::count_if(vCards.begin(), vCards.end(), [nCard](uint8_t& tCard) {
		return nCard == tCard;
	});
	auto nCardPre = make_Card_Num(nType, nValue == 3 ? nValue - 1 : nValue + 1);
	uint8_t nCntPre = std::count_if(vCards.begin(), vCards.end(), [nCardPre](uint8_t& tCard) {
		return nCardPre == tCard;
	});
	if (nCntPre < 1) {
		return false;
	}
	auto nCardAfter = make_Card_Num(nType, nValue == 3 ? nValue - 2 : nValue + 2);
	uint8_t nCntAfter = std::count_if(vCards.begin(), vCards.end(), [nCardAfter](uint8_t& tCard) {
		return nCardAfter == tCard;
	});
	if (nCntAfter < 1) {
		return false;
	}
	bool bFlag = false;
	auto nJiang = m_nJIang;
	removeHoldCard(nCard);
	removeHoldCard(nCardPre);
	removeHoldCard(nCardAfter);
	bFlag = MJPlayerCard::isHoldCardCanHu(m_nJIang);
	addHoldCard(nCard);
	addHoldCard(nCardPre);
	addHoldCard(nCardAfter);
	return bFlag;
}

bool NJMJPlayerCard::isQueYi() {
	std::map<uint8_t, uint8_t> vSign;
	for (uint8_t nIdx = 0; nIdx < eCT_Max; ++nIdx)
	{
		if (nIdx == eCT_Tiao || eCT_Wan == nIdx || eCT_Tong == nIdx)
		{
			if (m_vCards[nIdx].empty() == false)
			{
				vSign[nIdx] = 1;
			}
		}
	}

	for (auto& ref : m_vMingCardInfo)
	{
		if (ref.eAct == eMJAct_Peng || ref.eAct == eMJAct_AnGang || ref.eAct == eMJAct_BuGang || ref.eAct == eMJAct_MingGang) {
			auto eType = card_Type(ref.nTargetCard);
			if (eType == eCT_Tiao || eCT_Wan == eType || eCT_Tong == eType)
			{
				vSign[eType] = 1;
			}
		}
	}
	return vSign.size() == 2;
}

bool NJMJPlayerCard::getWaiBaoIdx(uint8_t& nBaoIdx, bool isZiMo) {
	auto nCnt = getHoldCardCnt();
	if (nCnt == 5) {
		if (checkQingYiSe() || checkDuiDuiHu()) {
			uint8_t nTempBao = -1;
			uint8_t nPengTimes = 0;
			uint8_t nCurPengTimes = 0;
			for (auto ref_ming : m_vMingCardInfo) {
				if (ref_ming.eAct == eMJAct_Peng || ref_ming.eAct == eMJAct_MingGang || ref_ming.eAct == eMJAct_BuGang) {
					nPengTimes++;
					if (nTempBao == -1) {
						nTempBao = ref_ming.nInvokerIdx;
						nCurPengTimes++;
					}
					else {
						if (nTempBao == ref_ming.nInvokerIdx) {
							nCurPengTimes++;
						}
						else {
							nTempBao == -1;
							nCurPengTimes = 0;
						}
					}
				}
				else if (ref_ming.eAct == eMJAct_AnGang) {
					nPengTimes++;
					nCurPengTimes++;
				}
			}

			if (nPengTimes == 3 && nCurPengTimes == 3) {
				if (isZiMo) {
					if (nTempBao != -1) {
						nBaoIdx = nTempBao;
						return true;
					}
				}
				else {
					nBaoIdx = nTempBao;
					return true;
				}
			}
		}
	}
	else if (nCnt == 2 && isZiMo == false) {
		if (isQQDD()) {
			return true;
		}
	}

	return false;
}

bool NJMJPlayerCard::getNormalBaoIdx(uint8_t& nBaoIdx, bool isZiMo) {
	if (isZiMo) {
		auto pPlayer = (NJMJPlayer*)getPlayer();
		if (pPlayer->getSongGangIdx() != -1) {
			nBaoIdx = pPlayer->getSongGangIdx();
			return true;
		}
	}
	else {
		auto pRoom = (NJMJRoom*)getPlayer()->getRoom();
		return pRoom->getQiangGangIdx() != -1;
	}
	return false;
}

bool NJMJPlayerCard::checkKuaiZhaoHu(uint8_t& nBaoIdx, std::vector<eFanxingType>& vFanxing) {
	if (m_bKuaiZhaoHu) {
		VEC_CARD vHoldCard, vCheckCard;
		getHoldCard(vHoldCard);
		auto nCard = getHuCard();
		if (vHoldCard.size() == 5) {
			vCheckCard.clear();
			vCheckCard.push_back(nCard);
			vCheckCard.push_back(nCard);
			vCheckCard.push_back(nCard);
			if (isHaveCards(vCheckCard)) {
				vCheckCard.clear();
				uint8_t nTempBao = -1;
				uint8_t nPengTimes = 0;
				uint8_t nCurPengTimes = 0;
				for (auto ref_ming : m_vMingCardInfo) {
					if (ref_ming.eAct == eMJAct_Peng || ref_ming.eAct == eMJAct_MingGang || ref_ming.eAct == eMJAct_BuGang) {
						vCheckCard.push_back(ref_ming.nTargetCard);
						nPengTimes++;
						if (nTempBao == -1) {
							nTempBao = ref_ming.nInvokerIdx;
							nCurPengTimes++;
						}
						else {
							if (nTempBao == ref_ming.nInvokerIdx) {
								nCurPengTimes++;
							}
							else {
								nTempBao == -1;
								nCurPengTimes = 0;
							}
						}
					}
					else if (ref_ming.eAct == eMJAct_AnGang) {
						vCheckCard.push_back(ref_ming.nTargetCard);
						nPengTimes++;
						nCurPengTimes++;
					}
				}
				if (nPengTimes == 3) {
					vFanxing.push_back(eFanxing_DuiDuiHu);
					bool bQYS = true;
					auto nType = card_Type(nCard);
					for (auto ref_check : vCheckCard) {
						auto nRefType = card_Type(ref_check);
						if (nRefType == nType) {
							continue;
						}
						bQYS = false;
						break;
					}
					if (bQYS) {
						vFanxing.push_back(eFanxing_QingYiSe);
					}

					if (nCurPengTimes == 3) {
						nBaoIdx = nTempBao == -1 ? m_nDianPaoIdx : nTempBao;

						if (nTempBao == -1 || nTempBao == m_nDianPaoIdx) {
							vFanxing.push_back(eFanxing_QuanQiuDuDiao);
						}
					}
					else if(bQYS){
						nBaoIdx = m_nDianPaoIdx;
						vFanxing.push_back(eFanxing_QuanQiuDuDiao);
					}

					return true;
				}
			}
		}
	}

	return false;
}

bool NJMJPlayerCard::checkKuaiZhaoZiMo(uint8_t& nBaoIdx, std::vector<eFanxingType>& vFanxing) {
	if (m_bKuaiZhaoZiMo) {
		VEC_CARD vHoldCard, vCheckCard;
		getHoldCard(vHoldCard);
		if (vHoldCard.size() == 5) {
			uint8_t nCard = getNewestFetchedCard();
			vCheckCard.push_back(nCard);
			vCheckCard.push_back(nCard);
			vCheckCard.push_back(nCard);
			if (isHaveCards(vCheckCard)) {
				vCheckCard.clear();
				uint8_t nTempBao = -1;
				uint8_t nPengTimes = 0;
				uint8_t nCurPengTimes = 0;
				for (auto ref_ming : m_vMingCardInfo) {
					if (ref_ming.eAct == eMJAct_Peng || ref_ming.eAct == eMJAct_MingGang || ref_ming.eAct == eMJAct_BuGang) {
						vCheckCard.push_back(ref_ming.nTargetCard);
						nPengTimes++;
						if (nTempBao == -1) {
							nTempBao = ref_ming.nInvokerIdx;
							nCurPengTimes++;
						}
						else {
							if (nTempBao == ref_ming.nInvokerIdx) {
								nCurPengTimes++;
							}
							else {
								nTempBao == -1;
								nCurPengTimes = 0;
							}
						}
					}
					else if (ref_ming.eAct == eMJAct_AnGang) {
						vCheckCard.push_back(ref_ming.nTargetCard);
						nPengTimes++;
						nCurPengTimes++;
					}
				}
				if (nPengTimes == 3 && nCurPengTimes == 3 && nTempBao != -1) {
					nBaoIdx = nTempBao;

					vFanxing.push_back(eFanxing_DuiDuiHu);
					vFanxing.push_back(eFanxing_QuanQiuDuDiao);

					bool bQYS = true;
					auto nType = card_Type(nCard);
					for (auto ref_check : vCheckCard) {
						auto nRefType = card_Type(ref_check);
						if (nRefType == nType) {
							continue;
						}
						bQYS = false;
						break;
					}
					if (bQYS) {
						vFanxing.push_back(eFanxing_QingYiSe);
					}

					return true;
				}
			}
		}
	}

	return false;
}

bool NJMJPlayerCard::checkQiDui(std::vector<eFanxingType>& vFanxing, bool isZiMo) {
	if (m_b7Pair) {
		uint8_t nCard = isZiMo ? getNewestFetchedCard() : getHuCard();
		VEC_CARD vHoldCard;
		getHoldCard(vHoldCard);
		auto nCnt = std::count(vHoldCard.begin(), vHoldCard.end(), nCard);
		if (nCnt < 4) {
			vFanxing.push_back(eFanxing_QiDui);
		}
		else {
			uint8_t nCnt4 = 1;
			uint8_t nTempCnt = 0;
			uint8_t nTempCard = 0;
			for (auto ref_card : vHoldCard) {
				if (ref_card == nCard) {
					nTempCnt = 0;
					nTempCard = 0;
					continue;
				}
				if (nTempCard) {
					if (ref_card == nTempCard) {
						nTempCnt++;
						if (nTempCnt == 4) {
							nCnt4++;
							nTempCnt = 0;
							nTempCard = 0;
						}
						continue;
					}
				}
				nTempCard = ref_card;
				nTempCnt = 1;
			}

			if (nCnt4 == 1) {
				vFanxing.push_back(eFanxing_ShuangQiDui);
			}
			else if (nCnt4 == 2) {
				vFanxing.push_back(eFanxing_DoubleShuangQiDui);
			}
			else{
				vFanxing.push_back(eFanxing_TribleShuangQiDui);
			}
		}

		return true;
	}
	return false;
}

bool NJMJPlayerCard::onBuHua(uint8_t nHuaCard, uint8_t nGetCard) {
	if (MJPlayerCard::onBuHua(nHuaCard, nGetCard)) {
		VEC_CARD vHuaCard;
		getBuHuaCard(vHuaCard);
		if (vHuaCard.size() > 3) {
			uint8_t nHuaCnt = 0;
			auto nHuaType = card_Type(nHuaCard);
			if (nHuaType == eCT_Jian) {
				nHuaCnt = std::count(vHuaCard.begin(), vHuaCard.end(), nHuaCard);
			}
			else {
				auto nHuaValue = card_Value(nHuaCard);
				uint8_t nMaxValue = nHuaValue < 5 ? 4 : 8;
				uint8_t nMinValue = nHuaValue < 5 ? 1 : 5;
				nHuaCnt = std::count_if(vHuaCard.begin(), vHuaCard.end(), [nMaxValue, nMinValue, nHuaType](const uint8_t& nCard) {
					if (card_Type(nCard) == nHuaType) {
						auto nValue = card_Value(nCard);
						return nValue <= nMaxValue && nValue >= nMinValue;
					}
					return false;
				});
			}
			if (nHuaCnt > 3) {
				auto pRoom = (NJMJRoom*)getPlayer()->getRoom();
				pRoom->onPlayerHuaGang(getPlayer()->getIdx(), nHuaCard);
			}
		}
		return true;
	}
	return false;
}