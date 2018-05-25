#pragma once
#include "ThirteenPrivateRoom.h"
#define MAX_ROOM_CNT 127
class ThirteenGPrivateRoom
	:public ThirteenPrivateRoom
{
public:
	struct stgStayPlayer
		:public stStayPlayer
	{
		stgStayPlayer() {
			nCurInIdx = -1;
			//nState = eNet_Online;
			bNeedDragIn = true;
			bWaitDragIn = false;
			bLeaved = true;
			bAutoStandup = false;
			bAutoLeave = false;
		}

		void reset() override {
			stStayPlayer::reset();
			bAutoStandup = false;
			bAutoLeave = false;
			nCurInIdx = -1;
			bLeaved = true;
			//nState = eNet_Offline;
		}

		uint16_t nCurInIdx;
		bool bNeedDragIn;
		bool bWaitDragIn;
		bool bLeaved;
		bool bAutoStandup;
		bool bAutoLeave;
		float bWaitDragInTime = 0;
	};
public:
	~ThirteenGPrivateRoom();
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)override;
	void setCurrentPointer(IGameRoom* pRoom)override;
	void packRoomInfo(Json::Value& jsRoomInfo)override;
	void onPlayerRotBanker(IGamePlayer* pPlayer, uint8_t nCoin)override;
	bool onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)override;
	void onPlayerWaitDragIn(uint32_t nUserUID)override;
	uint8_t checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)override;
	void doPlayerEnter(IGameRoom* pRoom, uint32_t nUserUID)override;
	void onPlayerWillSitDown(IGameRoom* pRoom, uint32_t nUserUID)override;
	void onPlayerSitDown(IGameRoom* pRoom, IGamePlayer* pPlayer)override;
	bool isRoomFull()override;
	void onPreGameDidEnd(IGameRoom* pRoom)override;
	void onGameDidEnd(IGameRoom* pRoom)override;
	bool doDeleteRoom()override;
	void onPlayerDoLeaved(IGameRoom* pRoom, uint32_t nUserUID)override;
	void update(float fDelta)override;
	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID) override;
	void sendRoomMsg(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nOmitSessionID = 0)override;
	void sendRoomMsgToAllPlayer(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nOmitSessionID = 0);
	void sendMsgToPlayer(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nSessionID)override;
	bool onPlayerNetStateRefreshed(uint32_t nPlayerID, eNetState nState) override;
	bool onPlayerSetNewSessionID(uint32_t nPlayerID, uint32_t nSessinID) override;
	void doSendRoomGameOverInfoToClient(bool isDismissed)override;
	void onPlayerApplyDragIn(uint32_t nUserUID, uint32_t nClubID)override;
	bool onPlayerDragIn(uint32_t nUserID, uint32_t nClubID, uint32_t nAmount)override;
	bool onPlayerDeclineDragIn(uint32_t nUserID)override;
	void onPlayerAutoStandUp(uint32_t nUserUID, bool bSwitch = true)override;
	void onPlayerAutoLeave(uint32_t nUserUID, bool bSwitch = true)override;
	void doRoomGameOver(bool isDismissed)override;
	bool canStartGame(IGameRoom* pRoom)override;
	uint16_t getPlayerCnt()override;
	uint16_t getMaxCnt() { return m_nMaxCnt; }
	void sendRealTimeRecord(uint32_t nSessionID = 0)override;

protected:
	virtual bool initMaxPlayerCnt();
	bool packTempRoomInfoToPlayer(stEnterRoomData* pEnterRoomPlayer);
	bool setCoreRoomBySessionID(uint32_t nSessionID);
	bool setCoreRoomByUserID(uint32_t nUserID);
	bool dispatcherPlayers(std::vector<stgStayPlayer*>& vWait);
	GameRoom* findWaitingRoom();

protected:
	std::vector<GameRoom*> m_vPRooms;

	uint16_t m_nMaxCnt;
	uint16_t m_nStartGameCnt = 6;

	/*
		Wait Queue
	*/
	float m_fWaitTime;

};