#pragma once
#include "GameRoom.h"
#include "IGameRoomDelegate.h"
class ICoinRoom
	:public IGameRoom
	, public IGameRoomDelegate
{
public:
	~ICoinRoom();
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)override;
	uint8_t checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)override;
	bool onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)override;
	bool isRoomFull()final;
	bool doDeleteRoom()override; // wanning: invoke by roomMgr ;
	virtual GameRoom* doCreatRealRoom() = 0;

	uint32_t getRoomID()final;
	uint32_t getSeiralNum()final;
	void update(float fDelta)final;
	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID) override;
	void sendRoomMsg(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nOmitSessionID = 0)final;
	void sendMsgToPlayer(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nSessionID)final;

	void packRoomInfo(Json::Value& jsRoomInfo)override;
	void sendRoomPlayersInfo(uint32_t nSessionID)override;
	void sendRoomInfo(uint32_t nSessionID)override;
	bool onPlayerNetStateRefreshed(uint32_t nPlayerID, eNetState nState) final;
	bool onPlayerSetNewSessionID(uint32_t nPlayerID, uint32_t nSessinID)final;
	uint16_t getPlayerCnt()override;
	GameRoom* getCoreRoom() { return m_pRoom; }

	IGamePlayer* getPlayerByIdx(uint16_t nIdx) override;
	uint16_t getSeatCnt() override;

	// delegate interface 
	void onStartGame(IGameRoom* pRoom)override;
	bool canStartGame(IGameRoom* pRoom)override;
	void onGameDidEnd(IGameRoom* pRoom)override;
	void onPlayerDoLeaved(IGameRoom* pRoom, uint32_t nUserUID)override;
private:
	GameRoom* m_pRoom;
};