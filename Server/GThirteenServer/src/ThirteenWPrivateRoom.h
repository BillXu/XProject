#pragma once
#include "ThirteenPrivateRoom.h"
class ThirteenWPrivateRoom
	:public ThirteenPrivateRoom {
public:
	struct stwStayPlayer
		:public stStayPlayer
	{
		//uint32_t nSessionID = 0;
		//uint32_t nUserUID;
		//uint32_t nClubID = 0;
		//int32_t nChip = 0;
		uint32_t nRebuyTime = 0;
		//int32_t nAllWrag = 0;
		//uint32_t isJoin = 0;
		bool bLeaved = true;
		bool bWaitDragIn = false;
		uint32_t nCurInIdx = -1;
		uint32_t tOutTime = 0; //淘汰时间
		uint32_t nOutGIdx = 0; //淘汰回合

		stwStayPlayer() {
			nSessionID = 0;
			nClubID = 0;
			nChip = 0;
			nAllWrag = 0;
			isJoin = 0;
		}

		void leave(){
			nSessionID = 0;
			//tOutTime = 0;
			//nOutGIdx = 0;
		}

		void tempLeave() {
			bLeaved = true;
			nCurInIdx = -1;
		}
	};
	struct mttLevelInfo
	{
		uint16_t nLevel;
		uint32_t nBaseScore;
		uint32_t nPreScore;
	};
	typedef std::map<uint32_t, stwStayPlayer*> MAP_UID_PLAYERS;
	typedef std::map<uint16_t, mttLevelInfo> MAP_MTT_LEVEL_INFO;
public:
	~ThirteenWPrivateRoom();
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)override;
	void setCurrentPointer(IGameRoom* pRoom)override;
	void packRoomInfo(Json::Value& jsRoomInfo)override;
	bool onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)override;
	void doPlayerEnter(IGameRoom* pRoom, uint32_t nUserUID)override;
	uint8_t checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)override;
	uint8_t getInitRound(uint8_t nLevel)override;
	bool canStartGame(IGameRoom* pRoom)override;
	void onPreGameDidEnd(IGameRoom* pRoom)override;
	void onGameDidEnd(IGameRoom* pRoom)override;
	bool isEnableReplay()override { return false; }
	void onPlayerApplyDragIn(uint32_t nUserUID, uint32_t nClubID)override;
	void onPlayerWillStandUp(IGameRoom* pRoom, IGamePlayer* pPlayer)override;
	void onPlayerStandedUp(IGameRoom* pRoom, uint32_t nUserUID)override;
	bool canPlayerSitDown(uint32_t nUserUID)override;
	void onPlayerSitDown(IGameRoom* pRoom, IGamePlayer* pPlayer)override;
	void onPlayerDoLeaved(IGameRoom* pRoom, uint32_t nUserUID)override;
	void onPlayerTempLeaved(IGameRoom* pRoom, stEnterRoomData* pEnterRoomPlayer)override;
	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override;
	bool onPlayerDragIn(uint32_t nUserID, uint32_t nClubID, uint32_t nAmount)override;
	bool onPlayerDeclineDragIn(uint32_t nUserID)override;
	uint8_t canPlayerDragIn(uint32_t nUserUID)override;
	void doRoomGameOver(bool isDismissed)override;
	bool doDeleteRoom()override;
	bool isRoomFull()override;
	void update(float fDelta)override;
	void onDismiss()override;
	bool onPlayerNetStateRefreshed(uint32_t nPlayerID, eNetState nState) override;
	bool onPlayerSetNewSessionID(uint32_t nPlayerID, uint32_t nSessinID) override;
	uint32_t getRoomPlayerCnt()override;
	uint16_t getPlayerCnt()override; //当前有多少人玩过
	uint16_t getMaxCnt() { return m_nMaxCnt; }
	uint32_t getBlindBaseScore()override;
	uint32_t getBlindPreScore()override;
	void onPlayerRotBanker(IGamePlayer* pPlayer, uint8_t nCoin)override;
	void onMTTPlayerCostPreScore(IGamePlayer* pPlayer)override;
	bool isPlayerDragIn(uint32_t nUserID);
	void sendRealTimeRecord(uint32_t nSessionID = 0)override;

protected:
	//void sendBssicRoomInfo(uint32_t nSessionID);
	//bool initMaxPlayerCnt()override;
	//bool packTempRoomInfoToPlayer(stEnterRoomData* pEnterRoomPlayer); //仅用于无比赛牌桌时的临时桌
	
																	  
	/*
		此方法返回是否需要额外发送房间信息，当玩家被正常分入房间后将需要依赖外部发送额外信息
		设计为只需要发送额外信息，玩家将被固定分配到有比赛的房间，当无比赛房间时主动派发到默认房间等待观战
		验证sessionID是否为0 是否在线
	*/
	//bool onDragInToGame(uint32_t nUserID);
	bool enterRoomToWatch(stEnterRoomData* pEnterRoomPlayer); //返回是否需要发送房间信息
	bool setCoreRoomBySessionID(uint32_t nSessionID);
	bool setCoreRoomByUserID(uint32_t nUserID);
	bool dispatcherPlayers(std::vector<stwStayPlayer*>& vWait);
	void dispatcherToEmptyPlace(std::vector<stwStayPlayer*>& vWait);
	GameRoom* findWaitingRoom();
	uint32_t getAliveCnt(); //当前有多少存活玩家
	uint32_t getEmptySeatCnt(IGameRoom* pRoom = nullptr);
	void initLevelInfo();
	void sendBssicRoomInfo(uint32_t nSessionID, uint32_t nUserID = 0)override;
	void onRoomStart();
	void sendRoomMsgToAllPlayer(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nOmitSessionID = 0);
	void pushRoomMsgToAllPlayer(Json::Value& prealMsg, uint16_t nMsgType, bool isDragIn = true);
	//stwStayPlayer* isEnterBySession(uint32_t nSessionID);
	//stwStayPlayer* isEnterByUserID(uint32_t nUserID);

protected:
	//bool m_isForbitEnterRoomWhenStarted;
	//uint8_t m_nAutoOpenCnt;
	//uint32_t m_nClubID = 0;
	//uint32_t m_nLeagueID = 0;
	uint32_t m_nStartTime = 0;
	bool m_bNeedVerify = true;
	uint32_t m_nInitialCoin = 0;
	uint32_t m_nRiseBlindTime = 0;
	uint8_t m_nRebuyLevel = 0;
	uint32_t m_nRebuyTime = 0;
	uint32_t m_nEnterFee = 0;
	uint8_t m_nDelayEnterLevel = 0;
	uint16_t m_nMaxCnt = 0;
	uint16_t m_nLestCnt = 0;

	uint8_t m_nCurBlind = 1;
	MAP_MTT_LEVEL_INFO m_mMTTLevelInfo;
	MAP_UID_PLAYERS m_mWatchPlayers;
	std::vector<GameRoom*> m_vPRooms;
	bool m_bNeedSplitRoom = false;

	CTimer m_tMTTBlindRise;
	//std::shared_ptr<IGameRoomRecorder> m_ptrRoomRecorder;
};