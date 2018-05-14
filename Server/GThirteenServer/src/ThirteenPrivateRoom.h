#pragma once
#include "IPrivateRoom.h"
class ThirteenPrivateRoom
	:public IPrivateRoom
{
public:
	struct stStayPlayer
	{
		uint32_t nSessionID;
		uint32_t nUserUID;
		uint32_t nClubID = 0;
		uint32_t nEnterClubID = 0;
		int32_t nChip;
		int32_t nAllWrag = 0;
		bool isSitdown = false;
		bool isDragIn = false;
		bool isTOut = false;
		uint32_t isJoin = 0;
		eNetState nState = eNet_Online;
		uint8_t nOffLineGame = 0;

		virtual void reset() {
			nState = eNet_Offline;
			nOffLineGame = 0;
		}
	};
	typedef std::map<uint32_t, stStayPlayer*> MAP_UID_PLAYERS;
public:
	~ThirteenPrivateRoom();
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)override;
	void packRoomInfo(Json::Value& jsRoomInfo)override;
	uint8_t checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)override;
	GameRoom* doCreatRealRoom()override;
	uint8_t getInitRound(uint8_t nLevel)override;
	void doSendRoomGameOverInfoToClient(bool isDismissed)override;
	bool canStartGame(IGameRoom* pRoom)override;
	//void onGameEnd(IGameRoom* pRoom)override;
	void onPlayerWillStandUp(IGameRoom* pRoom, IGamePlayer* pPlayer)override;
	void onPreGameDidEnd(IGameRoom* pRoom)override;
	void onGameDidEnd(IGameRoom* pRoom)override;
	bool isEnableReplay()override { return false; }
	void onPlayerWaitDragIn(uint32_t nUserUID)override;
	void onPlayerApplyDragIn(uint32_t nUserUID, uint32_t nClubID)override;
	bool onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)override;
	bool canPlayerSitDown(uint32_t nUserUID)override;
	void onPlayerSitDown(IGameRoom* pRoom, IGamePlayer* pPlayer)override;
	void onPlayerDoLeaved(IGameRoom* pRoom, uint32_t nUserUID)override;
	void onPlayerTOut(uint32_t nUserUID)override;
	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override;
	virtual bool onPlayerDragIn(uint32_t nUserID, uint32_t nClubID, uint32_t nAmount);
	virtual bool onPlayerDeclineDragIn(uint32_t nUserID);
	bool hasTreatedThisEvent(uint32_t nEventID);
	bool doDeleteRoom()override;
	bool isRoomGameOver()override;
	std::shared_ptr<IGameRoomRecorder> getRoomRecorder()override;
	uint32_t getRoomPlayerCnt()override;
	uint32_t getClubID()override;
	uint32_t getLeagueID()override;
	uint32_t getDragInClubID(uint32_t nUserID)override;
	uint32_t getEnterClubID(uint32_t nUserID)override;
	uint16_t getPlayerCnt()override;
	bool onPlayerSetNewSessionID(uint32_t nPlayerID, uint32_t nSessinID) override;
	void doRoomGameOver(bool isDismissed)override;
	void onPlayerRotBanker(IGamePlayer* pPlayer, uint8_t nCoin)override;
	uint32_t isClubRoom()override;
	bool onPlayerNetStateRefreshed(uint32_t nPlayerID, eNetState nState) override;
	void onDismiss()override;
	bool isMTT();
	uint32_t getEnterFee();
	uint32_t getInitialCoin();
	//void setCurrentPointer(IGameRoom* pRoom)override;

protected:
	virtual void sendBssicRoomInfo(uint32_t nSessionID, uint32_t nUserID = 0);
	stStayPlayer* isEnterBySession(uint32_t nSessionID);
	stStayPlayer* isEnterByUserID(uint32_t nUserID);

protected:
	bool m_isForbitEnterRoomWhenStarted;
	uint8_t m_nAutoOpenCnt;
	uint8_t m_nOverType;
	CTimer m_tCreateTimeLimit;
	MAP_UID_PLAYERS m_mStayPlayers;
	uint32_t m_nClubID = 0;
	uint32_t m_nLeagueID = 0;
	int32_t m_nRotBankerPool = 0;
	//std::vector<GameRoom*> m_vPRooms;

	std::shared_ptr<IGameRoomRecorder> m_ptrRoomRecorder;
	std::vector<uint32_t> m_vTreatedEvent;
};