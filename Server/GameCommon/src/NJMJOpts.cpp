#include "NJMJOpts.h"
#include "CommonDefine.h"
#include "log4z.h"
void NJMJOpts::initRoomOpts(Json::Value& jsOpts) {
	IMJOpts::initRoomOpts(jsOpts);

	m_bEnableJieZhuangBi = jsOpts["isJieZhuangBi"].asBool();
	m_bEnableHuaZa = jsOpts["isHuaZa"].asBool();
	m_bEnableSiLianFeng = jsOpts["isSiLianFeng"].asBool();
	m_bEnableWaiBao = jsOpts["isWaiBao"].asBool();
	m_bEnableBixiaHu = jsOpts["isBiXiaHu"].asBool();
	m_bEnableLeiBaoTa = jsOpts["isLeiBaoTa"].asBool();
	m_bEnableKuaiChong = jsOpts["isKuaiChong"].asBool();
	m_bEnableShuangGang = jsOpts["isShuangGang"].asBool();
	m_bEnableYiDuiDaoDi = jsOpts["isYiDuiDaoDi"].asBool();

	m_nGuang = 0;
	if (jsOpts["guang"].isUInt()) {
		m_nGuang = jsOpts["guang"].asUInt();
	}
	if (m_nGuang) {
		m_bEnableBaoMi = jsOpts["isBaoMi"].asBool();
	}
	m_nKuaiChongCoin = 0;
	if (jsOpts["kuaiChongCoin"].isUInt()) {
		m_nKuaiChongCoin = jsOpts["kuaiChongCoin"].asUInt();
	}
	m_nHuBaseScore = 10;
	if (jsOpts["huBaseScore"].isUInt()) {
		m_nHuBaseScore = jsOpts["huBaseScore"].isUInt();
		if (m_nHuBaseScore < 10) {
			m_nHuBaseScore = 10;
		}
		else {
			m_nHuBaseScore = m_nHuBaseScore / 10 * 10;
		}
	}
}

uint16_t NJMJOpts::calculateDiamondNeed() {
#ifdef _DEBUG
	return 0;
#endif // _DEBUG
	auto nLevel = getRoundLevel();
	if (nLevel > 2) {
		LOGFMTE("invalid room level for game = %u , level = %u", getGameType(), nLevel);
		nLevel = 0;
	}

	// is aa true ;
	if (isAA() || getPayType() == ePayType_AA) {
		if (isCircle()) {
			uint16_t vAA[3] = { 1 , 1 , 2 };
			return vAA[nLevel];
		}
		else {
			uint16_t vAA[3] = { 1 , 2 , 2 };
			return vAA[nLevel];
		}
	}
	else {
		if (isCircle()) {
			uint16_t vFangZhu[3] = { 2 , 4 , 8 };
			return vFangZhu[nLevel];
		}
		else {
			uint16_t vFangZhu[3] = { 3 , 5 , 8 };
			return vFangZhu[nLevel];
		}
	}
}

uint8_t NJMJOpts::calculateInitRound() {
#ifdef _DEBUG
	return 2;
#endif // _DEBUG

	auto nLevel = getRoundLevel();
	if (nLevel > 2) {
		LOGFMTE("invalid room level for game = %u , level = %u", getGameType(), nLevel);
		nLevel = 0;
	}

	if (isCircle()) {
		uint8_t vRounds[3] = {1, 2, 4};
		return vRounds[nLevel];
	}
	else {
		uint8_t vRounds[3] = {8, 12, 16};
		return vRounds[nLevel];
	}
}