#pragma once
#include "FanxingChecker.h"
#include "FanxingDuiDuiHu.h"
#include "FanxingQiangGang.h"
#include "FanxingQingYiSe.h"
#include "FanxingTianHu.h"
#include "FanxingHunYiSe.h"
#include "NJMJFanxingDiHu.h"
#include "FanxingQuanQiuDuDiao.h"
struct NJMJRoom::stSortFanInformation;
class NJMJFanxingChecker
	:public FanxingChecker
{
public:
	void init() {
		addFanxing(new FanxingDuiDuiHu());
		addFanxing(new FanxingQiangGang());
		addFanxing(new FanxingQingYiSe());
		addFanxing(new FanxingTianHu());
		addFanxing(new FanxingHunYiSe());
		addFanxing(new NJMJFanxingDiHu());
		addFanxing(new FanxingQuanQiuDuDiao());
	}

	bool checkFanxing(eFanxingType nType, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom) {
		if (m_vFanxing.count(nType)) {
			return m_vFanxing[nType]->checkFanxing(pPlayer->getPlayerCard(), pPlayer, nInvokerIdx, pmjRoom);
		}
		return false;
	}

	void checkFanxing(IMJPlayer* pPlayer, uint8_t nInvokerIdx, NJMJRoom::stSortFanInformation& stInformation);
	/*{
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
	}*/

protected:
	void eraseVectorOfAll(uint8_t p, std::vector<eFanxingType>& typeVec) {
		auto it = find(typeVec.begin(), typeVec.end(), p);
		while (it != typeVec.end())
		{
			typeVec.erase(it);
			it = find(typeVec.begin(), typeVec.end(), p);
		}
	}

	void sortHuHuaCnt(uint32_t& nFanCnt, std::vector<eFanxingType>& vFanxing);
};