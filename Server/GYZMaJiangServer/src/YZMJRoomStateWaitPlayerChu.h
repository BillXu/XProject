#pragma once
#include "MJRoomStateWaitPlayerChu.h"
class YZMJRoomStateWaitPlayerChu
	:public MJRoomStateWaitPlayerChu
{
public:
	void update(float fDeta)override
	{
		if (getWaitTime() > 15.0f) {
			auto pPlayer = (YZMJPlayer*)getRoom()->getPlayerByIdx(m_nIdx);
			pPlayer->addExtraTime(fDeta);
		}
		MJRoomStateWaitPlayerChu::update(fDeta);
	}
};