#pragma once
#include "IPokerOpts.h"
class NNOpts
	:public IPokerOpts
{
public:
	void initRoomOpts(Json::Value& jsOpts)override;
	uint16_t calculateDiamondNeed()override;
	uint8_t calculateInitRound()override;

	bool isEnableWuHua() { return m_bEnableWuHua; }
	bool isEnableZhaDan() { return m_bEnableZhaDan; }
	bool isEnableWuXiao() { return m_bEnableWuXiao; }
	bool isEnableShunKan() { return m_bEnableShunKan; }
	bool isEnableFengKuang() { return m_bEnableFengKuang; }
	bool isEnableTuiZhu() { return m_bEnableTuiZhu; }
	bool isEnableTuiZhuang() { return m_bEnableTuiZhuang; }
	uint8_t getBaseScore() { return m_nBaseScore; }
	uint8_t getBankType() { return m_nBankType; }
	uint8_t getFanBei() { return m_nFanBei; }

private:
	bool m_bEnableWuHua;
	bool m_bEnableZhaDan;
	bool m_bEnableWuXiao;
	bool m_bEnableShunKan;
	bool m_bEnableFengKuang;
	bool m_bEnableTuiZhu;
	bool m_bEnableTuiZhuang;
	uint8_t m_nBaseScore;
	uint8_t m_nBankType;
	uint8_t m_nFanBei;
};