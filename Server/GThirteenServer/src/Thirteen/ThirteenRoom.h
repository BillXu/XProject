#pragma once
#include "GameRoom.h"
#include "IPoker.h"
#include "ThirteenPoker.h"
class ThirteenRoom
	:public GameRoom
{
public:
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)override;
	IGamePlayer* createGamePlayer()override;
	void packRoomInfo(Json::Value& jsRoomInfo)override;
	void visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo, uint32_t nVisitorSessionID )override;
	uint8_t getRoomType()override;
	void onWillStartGame()override;
	void onStartGame()override;
	void onGameEnd()override;
	bool canStartGame()override;
	IPoker* getPoker()override;

	void onPlayerReady( uint16_t nIdx);
	void doDistributeCard( uint8_t nCardCnt );

	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override;
	bool onWaitAct();
	bool onWaitPlayerAct(uint8_t nIdx);
	bool onAutoDoPlayerAct();
	bool onPlayerSetDao(uint8_t nIdx, ThirteenPeerCard::VEC_CARD vCards);
	bool isWaitPlayerActForever() { return true; }
	bool isPlayerCanAct(uint8_t nIdx);
	bool isGameOver();
	uint8_t getBaseScore();
	uint8_t getMultiple();

protected:
	std::shared_ptr<IPlayerRecorder> createPlayerRecorderPtr()override;
	uint8_t getWinShui(uint8_t nIdx, uint8_t nWinType = WIN_SHUI_TYPE_NONE, uint8_t nDaoIdx = DAO_MAX);
	uint8_t getDaoWinShui(uint8_t nType, uint8_t nDaoIdx);
	uint32_t getWinCoin(uint8_t nIdx, uint8_t nWinType = WIN_SHUI_TYPE_NONE, uint8_t nDaoIdx = DAO_MAX);
private:
	ThirteenPoker m_tPoker;
};