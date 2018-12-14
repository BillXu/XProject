#pragma once 
#include "IPrivateRoom.h"
class MJPrivateRoom
	:public IPrivateRoom
{
 
public:
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)override;
	GameRoom* doCreatRealRoom()override;
	uint8_t getInitRound(uint8_t nLevel)override;
	void doSendRoomGameOverInfoToClient(bool isDismissed)override;
	bool canStartGame(IGameRoom* pRoom)override;
	bool isCircle();
	void decreaseLeftRound()override;
	bool applyDoDismissCheck()override;
	void packCreateUIDInfo(Json::Value& jsRoomInfo)override {}
	bool onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)override;
	void onPlayerSitDown(IGameRoom* pRoom, IGamePlayer* pPlayer)override;

protected:
	std::map<uint32_t, uint16_t> m_mPreSitIdxes;
};