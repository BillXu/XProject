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
#include "SCMJFanxingDiHu.h"
#include "SCMJFanxingGangHouPao.h"
#include "FanxingMenQing.h"
#include "SCMJFanxing19JiangDui.h"
#include "SCMJFanxingZhongZhang.h"
#include "FanxingHaiDiLaoYue.h"
#include "SCMJFanxingGen.h"
class SCMJFanxingChecker
	:public FanxingChecker
{
public:
	void init(bool have19, bool haveMenQing, bool haveZZ, bool haveTianHu, bool haveDiHu) {
		addFanxing(new Fanxing7Dui());
		addFanxing(new FanxingDuiDuiHu());
		addFanxing(new FanxingGangKai());
		addFanxing(new FanxingQiangGang());
		addFanxing(new FanxingQingYiSe());
		addFanxing(new FanxingQuanQiuDuDiao());
		addFanxing(new FanxingShuang7Dui());
		addFanxing(new SCMJFanxingGangHouPao());
		addFanxing(new FanxingHaiDiLaoYue());
		addFanxing(new SCMJFanxingGen());

		if (have19) {
			addFanxing(new SCMJFanxing19JiangDui());
		}
		if (haveMenQing) {
			addFanxing(new FanxingMenQing());
		}
		if (haveZZ) {
			addFanxing(new SCMJFanxingZhongZhang());
		}
		if (haveTianHu) {
			addFanxing(new FanxingTianHu());
		}
		if (haveDiHu) {
			addFanxing(new SCMJFanxingDiHu());
		}
	}

	void checkFanxing(std::vector<eFanxingType>& vFanxing, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override
	{
		bool isQiDuiOrDuiDui = false;
		for (auto& ref : m_vFanxing)
		{
			if (ref.first == eFanxing_SC_19JiangDui) {
				continue;
			}
			if (ref.first == eFanxing_SC_Gen) {
				((SCMJFanxingGen*)ref.second)->checkFanxing(pPlayer->getPlayerCard(), vFanxing);
				continue;
			}
			if (ref.second->checkFanxing(pPlayer->getPlayerCard(), pPlayer, nInvokerIdx, pmjRoom))
			{
				vFanxing.push_back((eFanxingType)ref.first);
				if (ref.first == eFanxing_QiDui || ref.first == eFanxing_DuiDuiHu) {
					isQiDuiOrDuiDui = true;
				}
				if (ref.first == eFanxing_ShuangQiDui) {
					eraseVectorOfAll(eFanxing_QiDui, vFanxing);
				}
			}
		}
		if (m_vFanxing.count(eFanxing_SC_19JiangDui)) {
			if (((SCMJFanxing19JiangDui*)m_vFanxing.at(eFanxing_SC_19JiangDui))->checkFanxing(pPlayer->getPlayerCard(), pPlayer, nInvokerIdx, pmjRoom, isQiDuiOrDuiDui)) {
				vFanxing.push_back(eFanxing_SC_19JiangDui);
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