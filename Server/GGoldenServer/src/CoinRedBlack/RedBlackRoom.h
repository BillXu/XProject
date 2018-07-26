#pragma once
#include "GameRoom.h"
#include "CommonDefine.h"
#include "Golden\GoldenPeerCard.h"
class RedBlackRoom
	:public GameRoom
{
public:
	struct stBetPool
	{
		int32_t nBetChips;
		CGoldenPeerCard tPeer;
		void clear()
		{
			nBetChips = 0;
			tPeer.reset();
		}
	};

	struct stRecorder
	{
		eBetPool eWinPort;
		CGoldenPeerCard::GoldenType eType;
	};
public:
	~RedBlackRoom();
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)override;
	IGamePlayer* createGamePlayer()override;
	IPoker* getPoker()override;
	uint8_t getRoomType()override;
	void onGameEnd()override;
	void onWillStartGame()override;
	void onStartGame()override;
	void onGameDidEnd()override;
	IGamePlayer* getPlayerByUID(uint32_t nUserUID)override;
	IGamePlayer* getPlayerBySessionID(uint32_t nSessionID)override;
	bool doPlayerSitDown(stEnterRoomData* pEnterRoomPlayer, uint16_t nIdx)override;
	bool doPlayerStandUp(uint32_t nUserUID)override;
	bool doPlayerLeaveRoom(uint32_t nUserUID)override;
	void sendRoomMsg(Json::Value& prealMsg, uint16_t nMsgType, uint32_t nOmitSessionID = 0)override;
	void packRoomInfo(Json::Value& jsRoomInfo)override;
	int32_t getCoinNeedToBeBanker();
	int32_t getWinRateForCardType( CGoldenPeerCard::GoldenType eType );
	void doDistribute();
protected:
	stBetPool m_vBetPool[eBet_Max];
	CGoldenPoker m_tPoker;
	std::vector<uint32_t> m_vApplyBanker;
	int32_t m_nBankerUID;
	std::map<uint32_t, IGamePlayer*> m_vStandGamePlayers;
	std::list<stRecorder> m_vRecorders;
};