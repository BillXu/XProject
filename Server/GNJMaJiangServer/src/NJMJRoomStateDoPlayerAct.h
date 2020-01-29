#pragma once
#include "MJRoomStateDoPlayerAct.h"
class NJMJRoomStateDoPlayerAct
	:public MJRoomStateDoPlayerAct
{
public:
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)override
	{
		m_nTing = 0;
		if (jsTranData["ting"].isUInt()) {
			m_nTing = jsTranData["ting"].asUInt();
		}

		MJRoomStateDoPlayerAct::enterState(pmjRoom, jsTranData);
	}

	void doAct() override
	{
		auto pRoom = ((NJMJRoom*)getRoom());
		switch (m_eActType)
		{
		case eMJAct_Chu:
		{
			if (m_nTing) {
				pRoom->onPlayerChu(m_nActIdx, m_nCard, m_nTing);
				return;
			}
		}
		break;
		}

		MJRoomStateDoPlayerAct::doAct();
	}

protected:
	uint8_t m_nTing;
};