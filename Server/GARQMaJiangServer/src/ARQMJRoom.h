#pragma once
#include "IMJRoom.h"
#include "IMJPoker.h"
#include "ARQMJFanxingChecker.h"
#include "ARQMJPoker.h"
class ARQMJRoom
	:public IMJRoom
{
public:
	struct stSettleGain
	{
	public:
		uint8_t nTargetIdx;
		uint32_t nGainChips;
	};

	struct stSettle
	{
		std::map<uint8_t, uint16_t> vWinIdxs;
		std::map<uint8_t, uint16_t> vLoseIdx;
		eMJActType eSettleReason;
		Json::Value jsHuMsg;
		void addWin(uint8_t nIdx, uint16_t nWinCoin)
		{
			if (vWinIdxs.count(nIdx)) {
				vWinIdxs[nIdx] += nWinCoin;
			}
			else {
				vWinIdxs[nIdx] = nWinCoin;
			}
			
		}

		void addLose(uint8_t nIdx, uint16_t nLoseCoin)
		{
			if (vLoseIdx.count(nIdx)) {
				vLoseIdx[nIdx] += nLoseCoin;
			}
			else {
				vLoseIdx[nIdx] = nLoseCoin;
			}
		}
	};
public:
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, std::shared_ptr<IGameOpts> ptrGameOpts)override;
	IGamePlayer* createGamePlayer()override;
	void packRoomInfo(Json::Value& jsRoomInfo)override;
	void visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo, uint32_t nVisitorSessionID)override;
	uint8_t getRoomType()override;
	void onWillStartGame()override;
	void onStartGame()override;
	void onGameEnd()override;
	IPoker* getPoker()override;
	bool isHaveLouPeng()override { return false; }
	bool isHaveLouHu()override { return false; }
	bool isGameOver()override;
	bool isRoomOver()override;
	bool isCanGoOnMoPai()override;
	bool canStartGame() override;
	bool onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)override;

	void doProduceNewBanker();
	void setNextBankerIdx(uint8_t nHuIdx = -1);
	uint8_t getBankerIdx() { return m_nBankerIdx; }
	bool isWaitPlayerActForever() { return true; }
	void onPlayerMo(uint8_t nIdx)override;
	void onPlayerChu(uint8_t nIdx, uint8_t nCard)override;
	void onPlayerEat(uint8_t nIdx, uint8_t nCard, uint8_t nWithA, uint8_t nWithB, uint8_t nInvokeIdx)override;
	void onPlayerPeng(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx)override;
	void onPlayerMingGang(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx)override;
	void onPlayerAnGang(uint8_t nIdx, uint8_t nCard)override;
	void onPlayerBuGang(uint8_t nIdx, uint8_t nCard)override;
	void onPlayerCyclone(uint8_t nIdx, uint8_t nCard);
	void onPlayerHu(std::vector<uint8_t>& vHuIdx, uint8_t nCard, uint8_t nInvokeIdx)override;
	void onWaitPlayerAct(uint8_t nIdx, bool& isCanPass)override;
	bool onWaitPlayerActAfterCP(uint8_t nIdx);
	void onAskForPengOrHuThisCard(uint8_t nInvokeIdx, uint8_t nCard, std::vector<uint16_t>& vOutWaitHuIdx, std::vector<uint16_t>& vOutWaitPengGangIdx, bool& isNeedWaitEat)override;
	bool isAnyPlayerRobotGang(uint8_t nInvokeIdx, uint8_t nCard)override;
	void onAskForRobotGang(uint8_t nInvokeIdx, uint8_t nCard, std::vector<uint8_t>& vOutCandinates)override;
	uint8_t getNextActPlayerIdx(uint8_t nCurActIdx)override;
	bool isPlayerRootDirectGang(uint8_t nInvokerIdx, uint8_t nCard);
	void onAskForRobotDirectGang(uint8_t nInvokeIdx, uint8_t nActIdx, uint8_t nCard, std::vector<uint8_t>& vOutCandinates);
	uint8_t checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)override { return 0; }
	uint8_t checkPlayerCanSitDown(stEnterRoomData* pEnterRoomPlayer)override;
	bool isRoomFull()override { return false; }

	uint8_t getFanLimit() { return 0; }
	bool isDPOnePay() { return false; }
	uint32_t getGuang() { return 0; }

	void addGain(uint8_t nIdx, stSettleGain stGain);
	void clearGain();
	void backGain(uint8_t nIdx);

	bool canGang() override;
	void onPrePlayerGang();
	bool needChu();

	void doRandomChangeSeat();
	bool doChangeSeat(uint16_t nIdx, uint16_t nWithIdx);
protected:
	void addSettle(stSettle& tSettle);
	void settleInfoToJson(Json::Value& jsRealTime);
	void sortFanxing2FanCnt(std::vector<eFanxingType>& vType, uint16_t& nFanCnt);
	void sendStartGameMsg();

protected:
	ARQMJPoker m_tPoker;
	std::vector<stSettle> m_vSettle;
	ARQMJFanxingChecker m_cFanxingChecker;

	uint8_t m_nNextBankerIdx;
	std::vector<std::vector<stSettleGain>> m_vGainChip;
	uint8_t m_nGangCnt;
	uint8_t m_nDice;
	uint8_t m_nR7;
	uint8_t m_nR15;

};