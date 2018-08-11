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
		uint8_t nKeyCardValue;
	};
public:
	~RedBlackRoom();
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)override;
	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID) override;
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
	int32_t getWinRateForCardType( CGoldenPeerCard::GoldenType eType, int8_t nPairKeyValue );
	void doDistribute();
	int32_t getBankerUID() { return m_nBankerUID; }
	int32_t getPoolCapacityToBet( eBetPool ePool );
	stBetPool& getPool( eBetPool ePool );
	bool onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)override;
	void informRichAndBestBetPlayerUpdate();
	void visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo, uint32_t nVisitorSessionID);
protected:
	int32_t getRichestPlayerUID();
	int32_t getBestBetPlayer();
protected:
	stBetPool m_vBetPool[eBet_Max];
	CGoldenPoker m_tPoker;
	std::vector<uint32_t> m_vApplyBanker;
	int32_t m_nBankerUID;
	std::map<uint32_t, IGamePlayer*> m_vStandGamePlayers;
	std::list<stRecorder> m_vRecorders;

	int32_t m_nRichestPlayer;  // da fu hao ;
	int32_t m_nBestBetPlayer;  // shen suan zi ;
	bool m_isWillLeaveBanker;
};