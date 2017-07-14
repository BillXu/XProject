#pragma once
#include "IGameRoom.h"
#include <vector>
class IGamePlayer;
class IGameRoomDelegate;
class GameRoom
	:public IGameRoom
{
public:
	struct stStandPlayer
	{
		uint32_t nUserUID;
		uint32_t nSessionID;
	};
public:
	~GameRoom();
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)override;
	void setDelegate(IGameRoomDelegate* pDelegate );
	uint8_t checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)override; // 0 can enter , 1 room is full ;
	bool onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)override;
	bool isRoomFull()final;

	virtual IGamePlayer* createGamePlayer() = 0;
	virtual void onWillStartGame();
	virtual void onStartGame();
	virtual bool canStartGame();
	virtual void onGameDidEnd();
	virtual void onGameEnd();
	virtual void packRoomInfo(Json::Value& jsRoomInfo) = 0;
	virtual void visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo) = 0;
	virtual bool doPlayerSitDown(stEnterRoomData* pEnterRoomPlayer,uint16_t nIdx );
	virtual bool doPlayerStandUp(uint32_t nUserUID);
	virtual bool doPlayerLeaveRoom( uint32_t nUserUID );

	uint32_t getRoomID()final;
	uint32_t getSeiralNum()final;
	void update(float fDelta)override;
	void sendRoomMsg(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nOmitSessionID = 0 )final;
	void sendMsgToPlayer(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nSessionID)final;
	bool onMsg(stMsg* prealMsg, eMsgPort eSenderPort, uint32_t nSessionID )override;
	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID )override;
	void sendRoomInfo(uint32_t nSessionID)final;
	IGameRoomManager* getRoomMgr();
	IGamePlayer* getPlayerByUID( uint32_t nUserUID );
	IGamePlayer* getPlayerBySessionID(uint32_t nSessionID);
	IGamePlayer* getPlayerByIdx( uint16_t nIdx );
	uint16_t getSeatCnt();
protected:
	IGameRoomDelegate* getDelegate();
	stStandPlayer* getStandPlayerBySessionID( uint32_t nSessinID );
	stStandPlayer* getStandPlayerByUID( uint32_t nUserID );
protected:
	IGameRoomDelegate* m_pDelegate;
	IGameRoomManager* m_pRoomMgr;
	Json::Value m_jsOpts;
	std::vector<IGamePlayer*> m_vPlayers;
	std::map<uint32_t,stStandPlayer*> m_vStandPlayers;
	uint32_t m_nRoomID;
	uint32_t m_nSieralNum;
};