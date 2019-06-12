#pragma once
#include "FanxingChecker.h"
#include "SDMJFanxing7Dui.h"
#include "SDMJFanxingDuiDuiHu.h"
#include "SDMJFanxingGangKai.h"
#include "FanxingQiangGang.h"
#include "SDMJFanxingQingYiSe.h"
#include "SDMJFanxingShuang7Dui.h"
#include "FanxingTianHu.h"
#include "SDMJFanxingHaiDiLaoYue.h"
#include "SDMJFanxingHunYiSe.h"
#include "FanxingDiHu.h"
#include "FanxingQuanQiuDuDiao.h"
#include "SDMJFanxingDaMenQing.h"
#include "SDMJFanxingXiaoMenQing.h"
class SDMJFanxingChecker
	:public FanxingChecker
{
public:
	void init() {
		addFanxing(new SDMJFanxing7Dui());
		addFanxing(new SDMJFanxingDuiDuiHu());
		addFanxing(new SDMJFanxingGangKai());
		addFanxing(new FanxingQiangGang());
		addFanxing(new SDMJFanxingQingYiSe());
		addFanxing(new SDMJFanxingShuang7Dui());
		addFanxing(new FanxingTianHu());
		addFanxing(new SDMJFanxingHaiDiLaoYue());
		addFanxing(new FanxingHunYiSe());
		addFanxing(new FanxingDiHu());
		addFanxing(new FanxingQuanQiuDuDiao());
		addFanxing(new SDMJFanxingDaMenQing());
		addFanxing(new SDMJFanxingXiaoMenQing());
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

		if (checkFanxing(eFanXing_SD_LuoDa, pPlayer, nInvokerIdx, pmjRoom)) {
			vFanxing.push_back(eFanXing_SD_LuoDa);
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