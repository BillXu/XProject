#pragma once
#include "IPrivateRoom.h"
class GoldenPrivateRoom
	:public IPrivateRoom
{
public:
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)override;
	uint8_t checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)override;
	GameRoom* doCreatRealRoom()override;
	uint8_t getInitRound(uint8_t nLevel)override;
	void doSendRoomGameOverInfoToClient(bool isDismissed)override;
	bool canStartGame(IGameRoom* pRoom)override;
	bool isEnableReplay()override { return true; }
protected:
	bool m_isForbitEnterRoomWhenStarted;
	uint8_t m_nAutoOpenCnt;
};