#pragma once
#include "IPrivateRoom.h"
class ThirteenPrivateRoom
	:public IPrivateRoom
{
public:
	struct stStayPlayer
	{
		uint32_t nUserUID;
		uint32_t nChip;
		uint8_t nCurInIdx;
	};
	typedef std::map<uint32_t, stStayPlayer> MAP_UID_PLAYERS;
public:
	//~ThirteenPrivateRoom();
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)override;
	void packRoomInfo(Json::Value& jsRoomInfo)override;
	uint8_t checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)override;
	GameRoom* doCreatRealRoom()override;
	uint8_t getInitRound(uint8_t nLevel)override;
	void doSendRoomGameOverInfoToClient(bool isDismissed)override;
	bool canStartGame(IGameRoom* pRoom)override;
	//void onGameEnd(IGameRoom* pRoom)override;
	void onPlayerWillStandUp(IGameRoom* pRoom, IGamePlayer* pPlayer)override;
	void onGameDidEnd(IGameRoom* pRoom)override;
	bool isEnableReplay()override { return true; }
	void onPlayerApplyDragIn(uint16_t nCnt)override;
	bool onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)override;
	void onPlayerSitDown(IGameRoom* pRoom, IGamePlayer* pPlayer)override;
	bool onPlayerDragIn(uint32_t nUserID, uint32_t nAmount);
	bool doDeleteRoom()override;
	bool isRoomGameOver()override;
	//void setCurrentPointer(IGameRoom* pRoom)override;
protected:
	bool m_isForbitEnterRoomWhenStarted;
	uint8_t m_nAutoOpenCnt;
	uint8_t m_nOverType;
	CTimer m_tCreateTimeLimit;
	MAP_UID_PLAYERS m_mStayPlayers;
	uint32_t m_nClubID = 0;
	uint32_t m_nLeagueID = 0;
	//std::vector<GameRoom*> m_vPRooms;
};