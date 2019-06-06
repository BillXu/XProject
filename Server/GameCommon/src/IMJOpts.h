#pragma once
#include "IGameOpts.h"
class IMJOpts 
	: public IGameOpts{
public:
	void initRoomOpts(Json::Value& jsOpts)override;
	uint32_t getBaseScore() { return m_nBaseScore; }
	bool isCircle() { return m_bCircle; }

private:
	uint32_t m_nBaseScore;
	bool m_bCircle;
};