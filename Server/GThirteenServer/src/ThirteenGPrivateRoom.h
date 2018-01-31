#pragma once
#include "ThirteenPrivateRoom.h"
#define MAX_ROOM_CNT 127
class ThirteenGPrivateRoom
	:public ThirteenPrivateRoom
{
public:
	~ThirteenGPrivateRoom();
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)override;
	void setCurrentPointer(IGameRoom* pRoom)override;
	bool onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)override;

protected:
	std::vector<GameRoom*> m_vPRooms;

	uint16_t m_nMaxCnt;
	uint16_t m_nStartGameCnt = 6;
};