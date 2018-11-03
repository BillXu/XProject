#pragma once
#include "FanxingChecker.h"
#include "Fanxing7Dui.h"
#include "FanxingDuiDuiHu.h"
#include "CFMJFanxingShuang7Dui.h"
#include "CFMJFanxing13Yao.h"
class CFMJFanxingChecker
	:public FanxingChecker
{
public:
	void init() {
		addFanxing(new FanxingDuiDuiHu());
		addFanxing(new Fanxing7Dui());
		addFanxing(new CFMJFanxingShuang7Dui());
		addFanxing(new CFMJFanxing13Yao());
	}

	bool checkFanxing(eFanxingType nType, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom) {
		if (m_vFanxing.count(nType)) {
			return m_vFanxing[nType]->checkFanxing(pPlayer->getPlayerCard(), pPlayer, nInvokerIdx, pmjRoom);
		}
		return false;
	}

	void checkFanxing(std::vector<eFanxingType>& vFanxing, IMJPlayer* pPlayer, uint8_t nInvokerIdx, IMJRoom* pmjRoom)override {
		bool is7Dui = false;
		auto pCard = pPlayer->getPlayerCard();
		for (auto& ref : m_vFanxing)
		{
			if (ref.first == eFanxing_ShuangQiDui) {
				if (!is7Dui) {
					continue;
				}
			}

			if (ref.second->checkFanxing(pCard, pPlayer, nInvokerIdx, pmjRoom))
			{
				vFanxing.push_back((eFanxingType)ref.first);
				if (ref.first == eFanxing_QiDui) {
					is7Dui = true;
				}
				else if (ref.first == eFanxing_ShuangQiDui) {
					eraseVectorOfAll(eFanxing_QiDui, vFanxing);
				}
			}
		}
	}

protected:
	void eraseVectorOfAll(eFanxingType p, std::vector<eFanxingType>& typeVec) {
		for (auto it = typeVec.begin(); it != typeVec.end();) {
			if (*it == p) {
				typeVec.erase(it++);
			}
			else {
				it++;
			}
		}
	}
};