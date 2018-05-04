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
		bool bJoin = false;
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
	};
	typedef std::map<uint32_t, stwStayPlayer*> MAP_UID_PLAYERS;
public:
	~ThirteenWPrivateRoom();
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)override;
	void setCurrentPointer(IGameRoom* pRoom)override;
	void packRoomInfo(Json::Value& jsRoomInfo)override;
	bool onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)override;
	uint8_t checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)override;
	uint8_t getInitRound(uint8_t nLevel)override;
	bool canStartGame(IGameRoom* pRoom)override;
	void onGameDidEnd(IGameRoom* pRoom)override;
	bool isEnableReplay()override { return false; }
	void onPlayerApplyDragIn(uint32_t nUserUID, uint32_t nClubID)override;
	bool canPlayerSitDown(uint32_t nUserUID)override;
	void onPlayerSitDown(IGameRoom* pRoom, IGamePlayer* pPlayer)override;
	void onPlayerDoLeaved(IGameRoom* pRoom, uint32_t nUserUID)override;
	void onPlayerTempLeaved(IGameRoom* pRoom, uint32_t nUserUID)override;
	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override;
	bool onPlayerDragIn(uint32_t nUserID, uint32_t nClubID, uint32_t nAmount)override;
	bool onPlayerDeclineDragIn(uint32_t nUserID)override;
	bool doDeleteRoom()override;
	bool isRoomFull()override;
	void update(float fDelta)override;
	bool onPlayerNetStateRefreshed(uint32_t nPlayerID, eNetState nState) override;
	bool onPlayerSetNewSessionID(uint32_t nPlayerID, uint32_t nSessinID) override;
	uint16_t getPlayerCnt()override;

protected:
	//void sendBssicRoomInfo(uint32_t nSessionID);
	//bool initMaxPlayerCnt()override;
	bool packTempRoomInfoToPlayer(stEnterRoomData* pEnterRoomPlayer); //仅用于无比赛牌桌时的临时桌
	bool enterRoomToWatch(stEnterRoomData* pEnterRoomPlayer); //返回是否需要发送房间信息
	bool setCoreRoomBySessionID(uint32_t nSessionID);
	bool setCoreRoomByUserID(uint32_t nUserID);
	//stwStayPlayer* isEnterBySession(uint32_t nSessionID);
	//stwStayPlayer* isEnterByUserID(uint32_t nUserID);

protected:
	//bool m_isForbitEnterRoomWhenStarted;
	//uint8_t m_nAutoOpenCnt;
	//uint32_t m_nClubID = 0;
	//uint32_t m_nLeagueID = 0;
	uint32_t m_nStartTime = 0;
	bool m_bNeedVerify = false;
	uint32_t m_nInitialCoin = 0;
	uint32_t m_nRiseBlindTime = 0;
	uint8_t m_nRebuyLevel = 0;
	uint32_t m_nRebuyTime = 0;
	uint32_t m_nEnterFee = 0;
	uint8_t m_nDelayEnterLevel = 0;
	uint16_t m_nMaxCnt = 0;

	std::vector<GameRoom*> m_vPRooms;
	//std::shared_ptr<IGameRoomRecorder> m_ptrRoomRecorder;
};