#pragma once
#include "IMJRoom.h"
#include "IMJPoker.h"
class TestMJ
	:public IMJRoom
{
public:
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, std::shared_ptr<IGameOpts> ptrGameOpts)override;
	IGamePlayer* createGamePlayer()override;
	uint8_t getRoomType()override;
	IPoker* getPoker()override;
protected:
	IMJPoker m_tPoker;
};