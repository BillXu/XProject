#pragma once
#include "MJRoomStateDoPlayerAct.h"
class AHMJRoomStateDoPlayerAct
	:public MJRoomStateDoPlayerAct
{
public:
	void onStateTimeUp() {
		switch (m_eActType)
		{
		case eMJAct_Chi:
		case eMJAct_Peng:
		{
			Json::Value jsValue;
			jsValue["idx"] = m_nActIdx;
			getRoom()->goToState(eRoomState_AfterChiOrPeng, &jsValue);
			break;
		}
		default:
			MJRoomStateDoPlayerAct::onStateTimeUp();
			break;
		}
	}
};