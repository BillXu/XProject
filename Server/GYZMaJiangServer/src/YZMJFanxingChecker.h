#pragma once
#include "FanxingChecker.h"
#include "YZMJFanxingDuiDuiHu.h"
#include "YZMJFanxing7Dui.h"
#include "YZMJFanxingFengQing.h"
#include "YZMJFanxingHunYiSe.h"
#include "YZMJFanxingQingYiSe.h"
#include "YZMJFanxingYiTiaoLong.h"
class YZMJFanxingChecker
	:public FanxingChecker
{
public:
	YZMJFanxingChecker() {
		init();
	}

	void init() {
		addFanxing(new YZMJFanxingDuiDuiHu());
		addFanxing(new YZMJFanxing7Dui());
		addFanxing(new YZMJFanxingFengQing());
		addFanxing(new YZMJFanxingHunYiSe());
		addFanxing(new YZMJFanxingQingYiSe);
		addFanxing(new YZMJFanxingYiTiaoLong);
	}

	bool checkFanxing(eFanxingType nType, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom) {
		if (m_vFanxing.count(nType)) {
			return m_vFanxing[nType]->checkFanxing(pPlayer->getPlayerCard(), pPlayer, nInvokerIdx, pmjRoom);
		}
		return false;
	}

	void checkFanxing(std::vector<eFanxingType>& vFanxing, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override {
		eFanxingType qiDuiType = eFanxing_PingHu;
		if (m_vFanxing.count(eFanxing_QiDui)) {
			auto qiDuiChecker = (YZMJFanxing7Dui*)m_vFanxing[eFanxing_QiDui];
			qiDuiChecker->checkFanxing(pPlayer->getPlayerCard(), qiDuiType);
		}
		if (qiDuiType == eFanxing_PingHu) {
			if (checkFanxing(eFanxing_YiTiaoLong, pPlayer, nInvokerIdx, pmjRoom)) {
				vFanxing.push_back(eFanxing_YiTiaoLong);
				if (checkFanxing(eFanxing_QingYiSe, pPlayer, nInvokerIdx, pmjRoom)) {
					vFanxing.push_back(eFanxing_QingYiSe);
				}
				else if (checkFanxing(eFanxing_HunYiSe, pPlayer, nInvokerIdx, pmjRoom)) {
					vFanxing.push_back(eFanxing_HunYiSe);
				}
			}
			else if (checkFanxing(eFanxing_FengQing, pPlayer, nInvokerIdx, pmjRoom)) {
				vFanxing.push_back(eFanxing_FengQing);
				if (checkFanxing(eFanxing_DuiDuiHu, pPlayer, nInvokerIdx, pmjRoom)) {
					vFanxing.push_back(eFanxing_DuiDuiHu);
				}
			}
			else if (checkFanxing(eFanxing_QingYiSe, pPlayer, nInvokerIdx, pmjRoom)) {
				vFanxing.push_back(eFanxing_QingYiSe);
				if (checkFanxing(eFanxing_DuiDuiHu, pPlayer, nInvokerIdx, pmjRoom)) {
					vFanxing.push_back(eFanxing_DuiDuiHu);
				}
			}
			else if (checkFanxing(eFanxing_DuiDuiHu, pPlayer, nInvokerIdx, pmjRoom)) {
				vFanxing.push_back(eFanxing_DuiDuiHu);
				if (checkFanxing(eFanxing_HunYiSe, pPlayer, nInvokerIdx, pmjRoom)) {
					vFanxing.push_back(eFanxing_HunYiSe);
				}
			}
			else if (checkFanxing(eFanxing_HunYiSe, pPlayer, nInvokerIdx, pmjRoom)) {
				vFanxing.push_back(eFanxing_HunYiSe);
			}
		}
		else {
			vFanxing.push_back(qiDuiType);

			if (checkFanxing(eFanxing_FengQing, pPlayer, nInvokerIdx, pmjRoom))
			{
				vFanxing.push_back(eFanxing_FengQing);
			}
			else if (checkFanxing(eFanxing_QingYiSe, pPlayer, nInvokerIdx, pmjRoom))
			{
				vFanxing.push_back(eFanxing_QingYiSe);
			}
			else if (checkFanxing(eFanxing_HunYiSe, pPlayer, nInvokerIdx, pmjRoom))
			{
				vFanxing.push_back(eFanxing_HunYiSe);
			}
		}

		if (vFanxing.empty()) {
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