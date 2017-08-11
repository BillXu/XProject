#pragma once
#include "GameRoom.h"
class IMJPlayer;
class IPoker;
class FanxingChecker;
#define MAX_SEAT_CNT 4 
class IMJRoom
	:public GameRoom
{
public:
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)override;
	bool onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)override;
public:
	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID) override;
	void onStartGame() override;
	void onGameEnd() override;
	void onGameDidEnd() override;
	bool canStartGame() override;

	uint8_t getBankerIdx();
	void setBankIdx(uint8_t nIdx);
	void onPlayerSetReady( uint8_t nIdx );
	// mj function ;
	virtual void onWaitPlayerAct(uint8_t nIdx, bool& isCanPass);
	virtual uint8_t getAutoChuCardWhenWaitActTimeout(uint8_t nIdx);
	virtual uint8_t getAutoChuCardWhenWaitChuTimeout(uint8_t nIdx);
	virtual void onPlayerMo(uint8_t nIdx);
	virtual void onPlayerPeng(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx);
	void onPlayerEat(uint8_t nIdx, uint8_t nCard, uint8_t nWithA, uint8_t nWithB, uint8_t nInvokeIdx);
	virtual void onPlayerMingGang(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx);
	virtual void onPlayerAnGang(uint8_t nIdx, uint8_t nCard);
	virtual void onPlayerBuGang(uint8_t nIdx, uint8_t nCard);
	virtual void onPlayerHu(std::vector<uint8_t>& vHuIdx, uint8_t nCard, uint8_t nInvokeIdx);
	virtual void onPlayerChu(uint8_t nIdx, uint8_t nCard);
	virtual bool isAnyPlayerPengOrHuThisCard( uint8_t nInvokeIdx , uint8_t nCard );
	virtual void onAskForPengOrHuThisCard(uint8_t nInvokeIdx, uint8_t nCard, std::vector<uint16_t>& vOutWaitHuIdx, std::vector<uint16_t>& vOutWaitPengGangIdx, bool& isNeedWaitEat);
	virtual bool isAnyPlayerRobotGang(uint8_t nInvokeIdx, uint8_t nCard);
	virtual void onAskForRobotGang(uint8_t nInvokeIdx, uint8_t nCard, std::vector<uint8_t>& vOutCandinates);
	virtual uint8_t getNextActPlayerIdx(uint8_t nCurActIdx);
	virtual bool isGameOver();
	virtual bool isCanGoOnMoPai();
	virtual void onPlayerLouHu( uint8_t nIdx , uint8_t nInvokerIdx );
	virtual void onPlayerLouPeng(uint8_t nIdx, uint32_t nLouCard );
	virtual bool isHaveLouHu() { return true; };
	virtual bool isHaveLouPeng() { return false; }
protected:
	uint8_t m_nBankerIdx;
	FanxingChecker* m_pFanxingChecker;
};