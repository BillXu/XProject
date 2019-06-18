#include "YZMJPlayerCard.h"
#include "IMJPoker.h"
#include "log4z.h"
#include "YZMJPlayer.h"
#include "YZMJRoom.h"
#include "YZMJFanxingChecker.h"

YZMJFanxingChecker YZMJPlayerCard::m_pFanxingChecker = YZMJFanxingChecker();
void YZMJPlayerCard::reset() {
	MJPlayerCard::reset();
	m_nHuCard = 0;
	m_bEnableHu = true;
	clearHuCnt();
}

void YZMJPlayerCard::onLouHu() {
	if (m_nLastHuCnt > m_nLouHuCnt) {
		m_nLouHuCnt = m_nLastHuCnt;
	}
}

void YZMJPlayerCard::clearHuCnt() {
	m_nLastHuCnt = 0;
	m_nLouHuCnt = 0;
	m_vLastHuFanxing.clear();
}

uint8_t YZMJPlayerCard::getLastHuCnt() {
	if (m_nLastHuCnt) {
		return m_nLastHuCnt - 1;
	}
	return 0;
}

void YZMJPlayerCard::getLastHuFanxing(std::vector<eFanxingType>& vFanxing) {
	vFanxing.insert(vFanxing.end(), m_vLastHuFanxing.begin(), m_vLastHuFanxing.end());
}

bool YZMJPlayerCard::onChuCard(uint8_t nChuCard) {
	if (m_bEnableHu && isHaveDa()) {
		m_bEnableHu = false;
	}
	return MJPlayerCard::onChuCard(nChuCard);
}

bool YZMJPlayerCard::onAnGang(uint8_t nCard, uint8_t nGangGetCard) {
	if (m_bEnableHu && isHaveDa()) {
		m_bEnableHu = false;
	}
	return onAnGang(nCard, nGangGetCard);
}

bool YZMJPlayerCard::canPengWithCard(uint8_t nCard) {
	if (nCard == getBaiDaCard()) {
		return false;
	}
	return MJPlayerCard::canPengWithCard(nCard);
}

bool YZMJPlayerCard::canMingGangWithCard(uint8_t nCard) {
	if (nCard == getBaiDaCard()) {
		return false;
	}
	return MJPlayerCard::canMingGangWithCard(nCard);
}

bool YZMJPlayerCard::canHuWitCard(uint8_t nCard) {
	if (m_bEnableHu) {
		auto pPlayer = (YZMJPlayer*)getPlayer();
		if (pPlayer->canBackGain()) {
			if (MJPlayerCard::canHuWitCard(nCard)) {
				return m_nLastHuCnt > m_nLouHuCnt;
			}
		}
	}
	return false;
}

bool YZMJPlayerCard::isHoldCardCanHu(uint8_t& nJiang) {
	bool bCanHu = MJPlayerCard::isHoldCardCanHu(nJiang);
	if (bCanHu) {
		m_vLastHuFanxing.clear();
		m_nLastHuCnt = 0;
		getFanXingAndFanCnt(m_vLastHuFanxing, m_nLastHuCnt);
	}
	return bCanHu;
}

bool YZMJPlayerCard::eraseVector(uint8_t p, VEC_CARD& typeVec)
{
	VEC_CARD::iterator it = find(typeVec.begin(), typeVec.end(), p);
	if (it != typeVec.end())
	{
		typeVec.erase(it);
		return true;
	}
	return false;
}

uint8_t YZMJPlayerCard::getHoldCardCnt() {
	VEC_CARD vHoldCard;
	getHoldCard(vHoldCard);
	return vHoldCard.size();
}

uint8_t YZMJPlayerCard::getBaiDaCard() {
	return ((YZMJRoom*)(getPlayer()->getRoom()))->getDa();
}

bool YZMJPlayerCard::isEanble7Pair() {
	return ((YZMJRoom*)(getPlayer()->getRoom()))->isQiDui();
}

uint8_t YZMJPlayerCard::isHaveDa() {
	if (getBaiDaCard()) {
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
	return 0;
}

uint8_t YZMJPlayerCard::daFilter(VEC_CARD& nCards) {
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

bool YZMJPlayerCard::vecHu7Pair(VEC_CARD vHuPai, uint8_t& eHunCnt) {
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

bool YZMJPlayerCard::checkYiTiaoLong() {
	//是否带一条龙玩法
	auto pRoom = (YZMJRoom*)getPlayer()->getRoom();
	if (pRoom->isYiTiaoLong() == false) {
		return false;
	}

	//获取所有牌
	std::vector<uint8_t> vHoldCards;
	getHoldCard(vHoldCards);

	//过滤掉所有搭并取得搭的数量
	uint8_t daCnt = daFilter(vHoldCards);

	//提取条筒万
	std::vector<VEC_CARD> vCardCopy;
	vCardCopy.resize(eCT_Max);
	for (auto nCard : vHoldCards)
	{
		auto eType = card_Type(nCard);
		if (eType > eCT_None || eType < eCT_Feng)
		{
			vCardCopy[eType].push_back(nCard);
		}
	}

	//获取一条龙
	for (uint8_t cardType = eCT_Wan; cardType < eCT_Feng; cardType++) {
		//最多4个搭，少于5张牌直接失败
		if (vCardCopy[cardType].size() < 5) {
			continue;
		}

		bool canHu = false;

		//拷贝手牌
		VEC_CARD backCards[eCT_Max];
		for (uint8_t nType = eCT_None; nType < eCT_Max; nType++) {
			backCards[nType].insert(backCards[nType].begin(), m_vCards[nType].begin(), m_vCards[nType].end());
		}

		do {
			//成一条龙需要的搭数
			uint8_t needDaCnt(0);

			//挨个获取1-9
			for (uint8_t cardNum = 1; cardNum < 10; cardNum++) {
				VEC_CARD::iterator it = std::find(vCardCopy[cardType].begin(), vCardCopy[cardType].end(), make_Card_Num((eMJCardType)cardType, cardNum));
				if (it == vCardCopy[cardType].end()) {
					//未获取到补一个搭
					needDaCnt++;
				}
				else {
					//获取到从非搭手牌删除
					removeHoldCard(*it);
				}
			}

			//需要太多搭失败
			if (needDaCnt > daCnt) {
				break;
			}

			//删除需要的搭
			while (needDaCnt > 0) {
				needDaCnt--;
				removeHoldCard(getBaiDaCard());
			}

			uint8_t nJiang = 0;
			canHu = MJPlayerCard::isHoldCardCanHu(nJiang);

		} while (false);

		//复原手牌
		for (uint8_t nType = eCT_None; nType < eCT_Max; nType++) {
			m_vCards[nType].clear();
			m_vCards[nType].insert(m_vCards[nType].begin(), backCards[nType].begin(), backCards[nType].end());
		}

		if (canHu) {
			return true;
		}
	}
	//未找到返回
	return false;
}

void YZMJPlayerCard::getFanXingAndFanCnt(std::vector<eFanxingType>& vHuTypes, uint8_t& nFanCnt) {
	m_pFanxingChecker.checkFanxing(vHuTypes, (IMJPlayer*)getPlayer(), 0, (IMJRoom*)getPlayer()->getRoom());
	sortFanCnt(vHuTypes, nFanCnt);
}

void YZMJPlayerCard::sortFanCnt(std::vector<eFanxingType>& vHuTypes, uint8_t& nFanCnt) {
	for (auto& ref : vHuTypes) {
		switch (ref)
		{
		case eFanxing_PingHu:
		{
			nFanCnt += 2;
		}
		break;
		case eFanxing_QiDui:
		case eFanxing_HunYiSe:
		case eFanxing_YiTiaoLong:
		case eFanxing_DuiDuiHu:
		{
			nFanCnt += 4;
		}
		break;
		case eFanxing_ShuangQiDui:
		case eFanxing_QingYiSe:
		{
			nFanCnt += 8;
		}
		break;
		case eFanxing_FengQing:
		{
			nFanCnt += 12;
		}
		break;
		case eFanxing_DoubleShuangQiDui:
		{
			nFanCnt += 16;
		}
		break;
		case eFanxing_TribleShuangQiDui:
		{
			nFanCnt += 32;
		}
		break;
		}
	}
}

void YZMJPlayerCard::onDoHu(uint16_t nInvokerIdx, uint8_t nHuCard, bool isInvokerHaveGangFlag)
{
	m_nDianPaoIdx = nInvokerIdx;
	onMoCard(nHuCard);
}