#pragma once
#include "ThirteenGPrivateRoom.h"
class ThirteenWPrivateRoom
	:public IPrivateRoom {
public:
	struct stwStayPlayer
	{
		uint32_t nSessionID = 0;
		uint32_t nUserUID;
		uint32_t nClubID = 0;
		int32_t nChip = 0;
		uint32_t nRebuyTime = 0;
		bool bJoin = false;
		bool bHadBeenJoined = false;
		uint32_t nCurInIdx = -1;
		uint32_t tOutTime = 0; //��̭ʱ��
		uint32_t nOutGIdx = 0; //��̭�غ�

		void reset(){
			tOutTime = 0;
			nOutGIdx = 0;
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
	bool isRoomFull()override;
	void onPlayerDoLeaved(IGameRoom* pRoom, uint32_t nUserUID)override;
	void update(float fDelta)override;
	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID) override;
	bool onPlayerNetStateRefreshed(uint32_t nPlayerID, eNetState nState) override;
	bool onPlayerSetNewSessionID(uint32_t nPlayerID, uint32_t nSessinID) override;
	uint16_t getPlayerCnt()override;

protected:
	bool initMaxPlayerCnt()override;
	bool packTempRoomInfoToPlayer(stEnterRoomData* pEnterRoomPlayer);
	bool enterRoomToWatch(stEnterRoomData* pEnterRoomPlayer); //�����Ƿ���Ҫ���ͷ�����Ϣ
	stwStayPlayer* isEnterBySession(uint32_t nSessionID);
	stwStayPlayer* isEnterByUserID(uint32_t nUserID);

protected:
	bool m_isForbitEnterRoomWhenStarted;
	uint8_t m_nAutoOpenCnt;
	uint32_t m_nClubID = 0;
	uint32_t m_nLeagueID = 0;
	uint32_t m_nStartTime = 0;
	bool m_bNeedVerify = false;
	uint32_t m_nInitialCoin = 0;
	uint32_t m_nRiseBlindTime = 0;
	uint8_t m_nRebuyLevel = 0;
	uint32_t m_nRebuyTime = 0;
	uint32_t m_nEnterFee = 0;
	uint8_t m_nDelayEnterLevel = 0;
	uint16_t m_nMaxCnt = 0;

	MAP_UID_PLAYERS m_mStayPlayers;
	std::vector<GameRoom*> m_vPRooms;
	std::shared_ptr<IGameRoomRecorder> m_ptrRoomRecorder;
};