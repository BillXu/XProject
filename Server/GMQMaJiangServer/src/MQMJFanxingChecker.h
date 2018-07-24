#pragma once
#include "FanxingChecker.h"
#include "Fanxing7Dui.h"
#include "FanxingDuiDuiHu.h"
#include "FanxingGangKai.h"
#include "FanxingQiangGang.h"
#include "FanxingQingYiSe.h"
#include "FanxingQuanQiuDuDiao.h"
#include "FanxingShuang7Dui.h"
#include "FanxingTianHu.h"
#include "MQMJFanxingGangHouPao.h"
#include "FanxingMenQing.h"
#include "MQMJFanxingHaiDiLaoYue.h"
#include "MQMJFanxingJiaHu.h"
#include "MQMJFanxingBianHu.h"
class MQMJFanxingChecker
	:public FanxingChecker
{
public:
	void init() {
		addFanxing(new FanxingDuiDuiHu());
		addFanxing(new FanxingGangKai());
		addFanxing(new MQMJFanxingJiaHu());
		addFanxing(new MQMJFanxingBianHu());
		addFanxing(new FanxingQiangGang());
		addFanxing(new MQMJFanxingGangHouPao());
		addFanxing(new MQMJFanxingHaiDiLaoYue());
		addFanxing(new FanxingMenQing());
	}

	bool checkFanxing(eFanxingType nType, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom) {
		if (m_vFanxing.count(nType)) {
			return m_vFanxing[nType]->checkFanxing(pPlayer->getPlayerCard(), pPlayer, nInvokerIdx, pmjRoom);
		}
		return false;
	}

	void checkFanxing(std::vector<eFanxingType>& vFanxing, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override {
		for (auto& ref : m_vFanxing)
		{
			if (ref.first == eFanxing_JiaHu && std::find(vFanxing.begin(), vFanxing.end(), eFanxing_BianHu) != vFanxing.end()) {
				continue;
			}
			if (ref.second->checkFanxing(pPlayer->getPlayerCard(), pPlayer, nInvokerIdx, pmjRoom))
			{
				vFanxing.push_back((eFanxingType)ref.first);
			}
		}
	}

protected:
	void eraseVectorOfAll(uint8_t p, std::vector<eFanxingType>& typeVec) {
		auto it = find(typeVec.begin(), typeVec.end(), p);
		while (it != typeVec.end())
		{
			typeVec.erase(it);
			it = find(typeVec.begin(), typeVec.end(), p);
		}
	}
};