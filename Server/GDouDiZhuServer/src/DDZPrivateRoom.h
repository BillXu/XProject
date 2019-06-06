#pragma once
#include "IPrivateRoom.h"
class DDZPrivateRoom
	:public IPrivateRoom
{
public:
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, std::shared_ptr<IGameOpts> ptrGameOpts)override;
	GameRoom* doCreatRealRoom()override;
	//uint8_t getInitRound(uint8_t nLevel)override;
	void doSendRoomGameOverInfoToClient(bool isDismissed)override;
	bool isEnableReplay()override { return true; }
	bool canStartGame(IGameRoom* pRoom);
	uint8_t checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)override;
protected:
	//uint8_t m_nAutoOpenCnt;
};