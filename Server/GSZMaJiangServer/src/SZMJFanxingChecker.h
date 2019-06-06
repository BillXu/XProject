#pragma once
#include "FanxingChecker.h"
#include "Fanxing7Dui.h"
#include "FanxingDuiDuiHu.h"
#include "SZMJFanxingGangKai.h"
#include "FanxingQiangGang.h"
#include "FanxingQingYiSe.h"
#include "SZMJFanxingShuang7Dui.h"
#include "FanxingTianHu.h"
#include "SZMJFanxingHaiDiLaoYue.h"
#include "FanxingHunYiSe.h"
#include "FanxingDiHu.h"
#include "FanxingQuanQiuDuDiao.h"
#include "SZMJFanxingDaMenQing.h"
#include "SZMJFanxingXiaoMenQing.h"
class SZMJFanxingChecker
	:public FanxingChecker
{
public:
	void init() {
		addFanxing(new Fanxing7Dui());
		addFanxing(new FanxingDuiDuiHu());
		addFanxing(new SZMJFanxingGangKai());
		addFanxing(new FanxingQiangGang());
		addFanxing(new FanxingQingYiSe());
		addFanxing(new SZMJFanxingShuang7Dui());
		addFanxing(new FanxingTianHu());
		addFanxing(new SZMJFanxingHaiDiLaoYue());
		addFanxing(new FanxingHunYiSe());
		addFanxing(new FanxingDiHu());
		addFanxing(new FanxingQuanQiuDuDiao());
		addFanxing(new SZMJFanxingDaMenQing());
		addFanxing(new SZMJFanxingXiaoMenQing());
	}

	bool checkFanxing(eFanxingType nType, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom) {
		if (m_vFanxing.count(nType)) {
			return m_vFanxing[nType]->checkFanxing(pPlayer->getPlayerCard(), pPlayer, nInvokerIdx, pmjRoom);
		}
		return false;
	}

	void checkFanxing(std::vector<eFanxingType>& vFanxing, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override {
		if (checkFanxing(eFanxing_TianHu, pPlayer, nInvokerIdx, pmjRoom)) {
			vFanxing.push_back(eFanxing_TianHu);
		}
		else if (checkFanxing(eFanxing_DiHu, pPlayer, nInvokerIdx, pmjRoom)) {
			vFanxing.push_back(eFanxing_DiHu);
		}
		
		if (checkFanxing(eFanxing_HaiDiLaoYue, pPlayer, nInvokerIdx, pmjRoom)) {
			vFanxing.push_back(eFanxing_HaiDiLaoYue);
		}

		if (checkFanxing(eFanxing_HunYiSe, pPlayer, nInvokerIdx, pmjRoom)) {
			vFanxing.push_back(eFanxing_HunYiSe);
		}
		else if (checkFanxing(eFanxing_QingYiSe, pPlayer, nInvokerIdx, pmjRoom)) {
			vFanxing.push_back(eFanxing_QingYiSe);
		}
		
		if (checkFanxing(eFanxing_DuiDuiHu, pPlayer, nInvokerIdx, pmjRoom)) {
			vFanxing.push_back(eFanxing_DuiDuiHu);
		}

		if (checkFanxing(eFanxing_ShuangQiDui, pPlayer, nInvokerIdx, pmjRoom)) {
			vFanxing.push_back(eFanxing_ShuangQiDui);
		}
		else if (checkFanxing(eFanxing_QiDui, pPlayer, nInvokerIdx, pmjRoom)) {
			vFanxing.push_back(eFanxing_QiDui);
		}
		else if (checkFanxing(eFanxing_DaMenQing, pPlayer, nInvokerIdx, pmjRoom)) {
			vFanxing.push_back(eFanxing_DaMenQing);
		}
		else if (checkFanxing(eFanxing_XiaoMenQing, pPlayer, nInvokerIdx, pmjRoom)) {
			vFanxing.push_back(eFanxing_XiaoMenQing);
		}

		if (checkFanxing(eFanxing_QuanQiuDuDiao, pPlayer, nInvokerIdx, pmjRoom)) {
			vFanxing.push_back(eFanxing_QuanQiuDuDiao);
		}

		if (vFanxing.empty())
		{
			vFanxing.push_back(eFanxing_PingHu);
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