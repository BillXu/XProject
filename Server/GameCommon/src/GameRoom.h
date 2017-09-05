#pragma once
#include "IGameRoom.h"
#include <vector>
#include "IGameRecorder.h"
#include "MJGameReplay.h"
class IGamePlayer;
class IGameRoomDelegate;
class IGameRoomState;
class IPoker;
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
	bool doDeleteRoom()override; // wanning: invoke by roomMgr ;

	virtual IGamePlayer* createGamePlayer() = 0;
	virtual void onWillStartGame();
	virtual void onStartGame();
	virtual bool canStartGame();
	virtual void onGameDidEnd();
	virtual void onGameEnd();
	virtual void packRoomInfo(Json::Value& jsRoomInfo) ;
	virtual void visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo, uint32_t nVisitorSessionID );
	virtual bool doPlayerSitDown(stEnterRoomData* pEnterRoomPlayer,uint16_t nIdx );
	virtual bool doPlayerStandUp(uint32_t nUserUID);
	virtual bool doPlayerLeaveRoom( uint32_t nUserUID );

	uint32_t getRoomID()final;
	uint32_t getSeiralNum()final;
	virtual uint8_t getRoomType() = 0 ;
	void update(float fDelta)override;
	void sendRoomMsg(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nOmitSessionID = 0 )final;
	void sendMsgToPlayer(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nSessionID)final;
	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID )override;
	void sendRoomInfo(uint32_t nSessionID)final;
	bool onPlayerNetStateRefreshed(uint32_t nPlayerID, eNetState nState)override;
	bool onPlayerSetNewSessionID(uint32_t nPlayerID, uint32_t nSessinID)final;
	IGameRoomManager* getRoomMgr();
	IGamePlayer* getPlayerByUID( uint32_t nUserUID );
	IGamePlayer* getPlayerBySessionID(uint32_t nSessionID);
	IGamePlayer* getPlayerByIdx( uint16_t nIdx );
	uint16_t getSeatCnt();

	virtual std::shared_ptr<IGameRoomRecorder> createRoomRecorder();
	virtual std::shared_ptr<ISingleRoundRecorder> createSingleRoundRecorder();
	IGameRoomState* getCurState();
	void goToState( IGameRoomState* pTargetState ,Json::Value* jsValue = nullptr);
	void goToState( uint32_t nStateID , Json::Value* jsValue = nullptr );
	void setInitState( IGameRoomState* pTargetState );
	virtual IPoker* getPoker() = 0;
	stStandPlayer* getStandPlayerBySessionID(uint32_t nSessinID);
protected:
	bool addRoomState(IGameRoomState* pTargetState);
	IGameRoomDelegate* getDelegate();
	stStandPlayer* getStandPlayerByUID( uint32_t nUserID );
	virtual bool addPlayerOneRoundOffsetToRecorder( IGamePlayer* pPlayer );  // signe player single round offset 
	bool addReplayFrame( uint32_t nFrameType , Json::Value& jsFrameArg );
private:
	std::shared_ptr<IGameRoomRecorder> getRoomRecorder();
protected:
	std::shared_ptr<ISingleRoundRecorder> getCurRoundRecorder();
protected:
	IGameRoomDelegate* m_pDelegate;
	IGameRoomManager* m_pRoomMgr;
	Json::Value m_jsOpts;
	std::vector<IGamePlayer*> m_vPlayers;
	std::map<uint32_t,stStandPlayer*> m_vStandPlayers;
	uint32_t m_nRoomID;
	uint32_t m_nSieralNum;
private:
	std::shared_ptr<ISingleRoundRecorder> m_ptrCurRoundRecorder;
	std::shared_ptr<IGameRoomRecorder> m_ptrRoomRecorder;
	std::shared_ptr<MJReplayGame> m_ptrGameReplay;
	IGameRoomState* m_pCurState;
	std::map<uint32_t, IGameRoomState*> m_vAllState;
};