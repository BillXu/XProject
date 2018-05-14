#pragma once
#include "GameRoom.h"
#include "IPoker.h"
#include "CardPoker.h"
class GoldenRoom
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
	uint8_t doProduceNewBanker();
	uint8_t getBankerIdx() { return m_nBankerIdx; }
	

	void doDistributeCard( uint8_t nCardCnt );

	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override;
	bool onWaitPlayerAct(uint8_t nIdx, bool& isCanPass);
	bool onPlayerPass(uint8_t nIdx, bool bForce = false);
	bool onPlayerCall(uint8_t nIdx);
	bool onPlayerCallToEnd(uint8_t nIdx);
	bool onPlayerAddCall(uint8_t nIdx, uint16_t nCoin);
	bool onPlayerKanPai(uint8_t nIdx);
	bool canPlayerPK(uint8_t nIdx);
	bool onPlayerPKWith(uint8_t nIdx, uint8_t nPKIdx);
	void onEndPK();
	bool isWaitPlayerActForever() { return true; }
	bool isPlayerCanAct(uint8_t nIdx);
	bool isGameOver();
	uint8_t getBaseScore();
	uint16_t getBaseStake();
	uint8_t getMultiple();
	uint8_t getNextMoveIdx(uint8_t nIdx);
	uint8_t getMustMenCircle();
	uint8_t getCanPKCircle();

	bool isEnable235();
	bool isEnableStraight();
	bool isEnableXiQian();
	uint8_t getCircleLimit();
	uint8_t getPKTimes();

protected:
	std::shared_ptr<IPlayerRecorder> createPlayerRecorderPtr()override;
	uint16_t getCallCoin();
	uint8_t getCallMutiple(uint16_t nCoin);
private:
	uint8_t m_nBankerIdx;
	uint8_t m_nLastWinIdx;

	/*
		底分目前暂定1,2,3,4,5
		倍数暂定10,20,30,40,50
		下注分数为：
			如果是闷：底分 * 倍数 / 10
			如果是明牌： 底分 * 倍数 / 5
	*/
	uint16_t m_aCallScore[5];

	uint8_t m_nCurCircle; //当前圈数
	uint8_t m_nCurMutiple; //当前跟注倍数
	uint32_t m_nGoldPool; //金币池

	CGoldenPoker m_tPoker;
};