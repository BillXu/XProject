#pragma once
#include "IMJOpts.h"
class YZMJOpts
	:public IMJOpts
{
public:
	void initRoomOpts(Json::Value& jsOpts)override;
	uint16_t calculateDiamondNeed()override;
	uint8_t calculateInitRound()override;

	uint32_t getGuang() { return m_nGuang; }
	bool isPeiZi() { return m_bPeiZi; }
	bool isBaiBanPeiZi() { return m_bPeiZi && m_bBaiBanPeiZi; }
	bool isYiTiaoLong() { return m_bYiTiaoLong; }
	bool isQiDui() { return m_bQiDui; }

private:
	uint32_t m_nGuang;
	bool m_bPeiZi;
	bool m_bBaiBanPeiZi;
	bool m_bYiTiaoLong;
	bool m_bQiDui;
};