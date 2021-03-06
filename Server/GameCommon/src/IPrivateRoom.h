#pragma once 
#include "Timer.h"
#include "GameRoom.h"
#include "IGameRoomDelegate.h"
class IPrivateRoom
	:public IGameRoom
	,public IGameRoomDelegate
{
public:
	enum eState
	{
		eState_WaitStart,
		eState_Started,
		eState_RoomOvered ,
		eState_Max,
	};
public:
	~IPrivateRoom();
	bool init( IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts )override;
	uint8_t checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)override;
	bool onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)override;
	bool isRoomFull()final;
	bool doDeleteRoom()override; // wanning: invoke by roomMgr ;

	uint32_t getRoomID()final;
	uint32_t getSeiralNum()final;
	void update(float fDelta)final;
	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID) override;
	void sendRoomMsg(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nOmitSessionID = 0)final;
	void sendMsgToPlayer(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nSessionID)final;

	virtual GameRoom* doCreatRealRoom() = 0;
	uint8_t getDiamondNeed( uint8_t nLevel, ePayRoomCardType nPayType );
	virtual uint8_t getInitRound(uint8_t nLevel) = 0;

	void packRoomInfo(Json::Value& jsRoomInfo)override;
	void sendRoomPlayersInfo(uint32_t nSessionID)override;
	void sendRoomInfo(uint32_t nSessionID)override;
	bool onPlayerNetStateRefreshed(uint32_t nPlayerID, eNetState nState) final;
	bool onPlayerSetNewSessionID(uint32_t nPlayerID, uint32_t nSessinID)final;

	// delegate interface 
	void onStartGame(IGameRoom* pRoom)override;
	bool canStartGame(IGameRoom* pRoom)override;
	void onGameDidEnd(IGameRoom* pRoom)override;
	bool isEnableRecorder()final { return true; }
	bool isEnableReplay()override { return true; }

	void doRoomGameOver(bool isDismissed);
	virtual void doSendRoomGameOverInfoToClient( bool isDismissed ) = 0;
	GameRoom* getCoreRoom();
	uint16_t getPlayerCnt()override;
	bool isClubRoom() { return m_nClubID > 0; }
	uint32_t getClubID() { return m_nClubID; }
protected:
	bool isRoomStarted();
	bool isOneRoundNormalEnd();
	bool onProcessWhiteListSitDown(Json::Value& prealMsg,uint32_t nSessionID );
	bool isRoomOwnerPay() { return ePayType_RoomOwner == m_nPayType; }
	bool isAAPay() { return ePayType_AA == m_nPayType;  }
	bool isWinerPay() { return ePayType_Winer == m_nPayType; }
	ePayRoomCardType getPayType() { return m_nPayType; }
	void doProcessWinerPayRoomCard();
protected:
	IGameRoomManager* m_pRoomMgr;
	uint32_t m_nOwnerUID;
	uint32_t m_nClubID;
	ePayRoomCardType m_nPayType;
	bool m_isEnableWhiteList;

	bool m_isOpen; 

	uint8_t m_nRoundLevel; // round level , comsume card and init round deponeded on this level ;
	uint8_t m_nLeftRounds;

	bool m_isOneRoundNormalEnd;
	eState m_nPrivateRoomState;

	bool m_bWaitDismissReply;
	std::map<uint32_t, uint8_t> m_vPlayerAgreeDismissRoom; // key : uid , value: replayer , 0  agree , 1 disagree ;
	CTimer m_tWaitReplyDismissTimer;
	time_t m_tInvokerTime;
	uint32_t m_nApplyDismissUID;

	GameRoom* m_pRoom;

	CTimer m_tAutoDismissTimer;
};