#pragma once
#include "FanxingChecker.h"
#include "FanxingDuiDuiHu.h"
#include "AHMJFanxingMenQing.h"
#include "AHMJFanxingJiaHu.h"
class AHMJFanxingChecker
	:public FanxingChecker
{
public:
	AHMJFanxingChecker() {
		init();
	}

	void init() {
		addFanxing(new FanxingDuiDuiHu());
		addFanxing(new AHMJFanxingJiaHu());
	}

	void checkFanxing(std::vector<eFanxingType>& vFanxing, IMJPlayerCard* pCard, uint8_t nInvokerIdx, IMJRoom* pmjRoom) {
		for (auto& ref : m_vFanxing)
		{
			if (ref.second->checkFanxing(pCard, nullptr, nInvokerIdx, pmjRoom))
			{
				vFanxing.push_back((eFanxingType)ref.first);
			}
		}
	}
};