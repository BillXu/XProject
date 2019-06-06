#pragma once
#include "IGameOpts.h"
class IPokerOpts
	:public IGameOpts
{
public:
	void initRoomOpts(Json::Value& jsOpts)override {
		IGameOpts::initRoomOpts(jsOpts);

		m_bForbitEnterRoomWhenStarted = jsOpts["forbidJoin"].asBool();
	}

	bool isForbitEnterRoomWhenStarted() { return m_bForbitEnterRoomWhenStarted; }

private:
	bool m_bForbitEnterRoomWhenStarted;
};