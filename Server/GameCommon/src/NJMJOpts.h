#pragma once
#include "IMJOpts.h"
class NJMJOpts
	:public IMJOpts
{
public:
	void initRoomOpts(Json::Value& jsOpts)override;
	uint16_t calculateDiamondNeed()override;
	uint8_t calculateInitRound()override;

	uint32_t getGuang() { return m_nGuang; }
	bool isEnableJieZhuangBi() { return m_bEnableJieZhuangBi; }
	bool isEnableHuaZa() { return m_bEnableHuaZa; }
	bool isEnableSiLianFeng() { return m_bEnableSiLianFeng; }
	bool isEnableWaiBao() { return getGuang() ? m_bEnableWaiBao : false; }
	bool isEnableBixiaHu() { return m_bEnableBixiaHu; }
	bool isEnableLeiBaoTa() { return m_bEnableLeiBaoTa && m_bEnableBixiaHu; }
	bool isEnableKuaiChong() { return m_bEnableKuaiChong; }
	bool isEnableBaoMi() { return m_bEnableBaoMi; }
	uint32_t getKuaiChongCoin() { return m_nKuaiChongCoin; }
	bool isEnableShuangGang() { return getGuang() ? false : m_bEnableShuangGang; }
	bool isEnableYiDuiDaoDi() { return getGuang() ? false : m_bEnableYiDuiDaoDi; }

private:
	uint32_t m_nGuang;
	uint32_t m_nKuaiChongCoin;

	bool m_bEnableJieZhuangBi;
	bool m_bEnableHuaZa;
	bool m_bEnableSiLianFeng;
	bool m_bEnableWaiBao;
	bool m_bEnableBixiaHu;
	bool m_bEnableLeiBaoTa;
	bool m_bEnableKuaiChong;
	bool m_bEnableBaoMi;
	bool m_bEnableShuangGang;
	bool m_bEnableYiDuiDaoDi;
};