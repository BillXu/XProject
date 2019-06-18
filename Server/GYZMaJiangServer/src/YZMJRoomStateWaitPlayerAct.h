#pragma once
#include "MJRoomStateWaitPlayerAct.h"
#include "YZMJRoom.h"
class YZMJRoomStateWaitPlayerAct
	:public MJRoomStateWaitPlayerAct
{
public:
	void update(float fDeta)override
	{
		if (getWaitTime() > 15.0f) {
			auto pPlayer = (YZMJPlayer*)getRoom()->getPlayerByIdx(m_nIdx);
			pPlayer->addExtraTime(fDeta);
		}
		MJRoomStateWaitPlayerAct::update(fDeta);
	}
};