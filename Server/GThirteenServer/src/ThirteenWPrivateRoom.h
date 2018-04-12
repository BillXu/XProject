#pragma once
#include "ThirteenGPrivateRoom.h"
class ThirteenWPrivateRoom
	:public ThirteenGPrivateRoom {
public:
	struct stwStayPlayer
		:public stgStayPlayer
	{
		uint32_t tOutTime;
		uint32_t nOutGIdx;

		stwStayPlayer() {
			tOutTime = 0;
			nOutGIdx = 0;
			nState = eNet_Offline;
		}

		void reset() override {
			stwStayPlayer::reset();
			tOutTime = 0;
			nOutGIdx = 0;
		}
	};

public:
	~ThirteenWPrivateRoom();
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)override;
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
	uint32_t m_nStartTime = 0;
};