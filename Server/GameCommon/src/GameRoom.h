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

	class stLastActInfo
	{
	public:
		stLastActInfo() {
			nActIdx = -1;
			nInvokerIdx = -1;
			nActType = -1;
			nCardInfo = 0;
		}

		stLastActInfo(uint8_t actIdx, uint8_t invokerIdx, uint16_t actType, uint16_t cardInfo) {
			nActIdx = actIdx;
			nInvokerIdx = invokerIdx;
			nActType = actType;
			nCardInfo = cardInfo;
		}

		~stLastActInfo();

	public:
		uint8_t nActIdx;
		uint8_t nInvokerIdx;
		uint16_t nActType;
		uint16_t nCardInfo;
	};

public:
	~GameRoom();
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, std::shared_ptr<IGameOpts> ptrGameOpts)override;
	void setDelegate(IGameRoomDelegate* pDelegate );
	uint8_t checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)override; // 0 can enter , 1 room is full ;
	virtual uint8_t checkPlayerCanSitDown(stEnterRoomData* pEnterRoomPlayer) { return 0; }
	bool onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)override;
	bool isRoomFull()override;
	bool doDeleteRoom()override; // wanning: invoke by roomMgr ;

	virtual IGamePlayer* createGamePlayer() = 0;
	virtual void onWillStartGame();
	virtual void onStartGame();
	virtual bool canStartGame();
	virtual void onGameDidEnd();
	virtual void onGameEnd();
	void sendRoomPlayersInfo(uint32_t nSessionID)override;
	void packRoomInfo(Json::Value& jsRoomInfo)override ;
	virtual void visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo, uint32_t nVisitorSessionID );
	virtual bool doPlayerSitDown(stEnterRoomData* pEnterRoomPlayer,uint16_t nIdx );
	virtual bool doPlayerStandUp(uint32_t nUserUID);
	virtual bool doPlayerLeaveRoom( uint32_t nUserUID );
	virtual bool isOneCircleEnd() { return true; }

	uint32_t getRoomID()final;
	uint32_t getSeiralNum()final;
	virtual uint8_t getRoomType() = 0 ;
	void update(float fDelta)override;
	void sendRoomMsg(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nOmitSessionID = 0 )final;
	void sendRoomMsgToStander(Json::Value& prealMsg, uint16_t nMsgType);
	void sendMsgToPlayer(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nSessionID)final;
	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID )override;
	void sendRoomInfo(uint32_t nSessionID)override;
	bool onPlayerNetStateRefreshed(uint32_t nPlayerID, eNetState nState)override;
	bool onPlayerSetNewSessionID(uint32_t nPlayerID, uint32_t nSessinID)final;
	IGameRoomManager* getRoomMgr();
	IGamePlayer* getPlayerByUID( uint32_t nUserUID );
	IGamePlayer* getPlayerBySessionID(uint32_t nSessionID);
	IGamePlayer* getPlayerByIdx( uint16_t nIdx )final;
	uint16_t getSeatCnt()final;

	virtual std::shared_ptr<IGameRoomRecorder> createRoomRecorder();
	virtual std::shared_ptr<ISingleRoundRecorder> createSingleRoundRecorder();
	IGameRoomState* getCurState();
	void goToState( IGameRoomState* pTargetState ,Json::Value* jsValue = nullptr);
	void goToState( uint32_t nStateID , Json::Value* jsValue = nullptr );
	void setInitState( IGameRoomState* pTargetState );
	virtual IPoker* getPoker() = 0;
	stStandPlayer* getStandPlayerBySessionID(uint32_t nSessinID);
	uint16_t getPlayerCnt()override;
	void setTempID(uint32_t nTempID) { m_nTempID = nTempID; }
	uint32_t getTempID() { return m_nTempID; }

	bool checkPlayerInThisRoom(uint32_t nSessionID)override;

	void saveGameRecorder(bool bDismiss = false);
	virtual bool isHaveRace() { return false; }
	virtual void onWaitRace(uint8_t nIdx = -1) {}
	void regesterLastAct(std::auto_ptr<stLastActInfo> ptrLastActInfo) { m_ptrLastActInfo = ptrLastActInfo; }
protected:
	bool addRoomState(IGameRoomState* pTargetState);
	IGameRoomDelegate* getDelegate();
	stStandPlayer* getStandPlayerByUID( uint32_t nUserID );
	virtual std::shared_ptr<IPlayerRecorder> createPlayerRecorderPtr();
public:
	bool addReplayFrame( uint32_t nFrameType , Json::Value& jsFrameArg );
private:
	std::shared_ptr<IGameRoomRecorder> getRoomRecorder();
protected:
	std::shared_ptr<ISingleRoundRecorder> getCurRoundRecorder();
protected:
	IGameRoomDelegate* m_pDelegate;
	IGameRoomManager* m_pRoomMgr;
	//Json::Value m_jsOpts;
	std::vector<IGamePlayer*> m_vPlayers;
	std::map<uint32_t,stStandPlayer*> m_vStandPlayers;
	uint32_t m_nRoomID;
	uint32_t m_nSieralNum;
	uint32_t m_nTempID;
private:
	std::shared_ptr<ISingleRoundRecorder> m_ptrCurRoundRecorder;
	std::shared_ptr<IGameRoomRecorder> m_ptrRoomRecorder;
	std::shared_ptr<MJReplayGame> m_ptrGameReplay;
	IGameRoomState* m_pCurState;
	std::map<uint32_t, IGameRoomState*> m_vAllState;

	std::auto_ptr<stLastActInfo> m_ptrLastActInfo;

	bool m_bSaveRecorder = true;
};