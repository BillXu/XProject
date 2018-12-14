#pragma once
#include "IMJRoom.h"
#include "IMJPoker.h"
#include "FXMJFanxingChecker.h"
#include "FXMJPoker.h"
class FXMJRoom
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
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)override;
	IGamePlayer* createGamePlayer()override;
	void packRoomInfo(Json::Value& jsRoomInfo)override;
	void visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo, uint32_t nVisitorSessionID)override;
	uint8_t getRoomType()override;
	void onWillStartGame()override;
	void onStartGame()override;
	void onGameEnd()override;
	IPoker* getPoker()override;
	bool isHaveLouPeng()override { return true; }
	bool isGameOver()override;
	bool isRoomOver()override;
	bool isCanGoOnMoPai()override;
	bool canStartGame() override;
	bool onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)override;
	void onPlayerLouHu(uint8_t nIdx, uint8_t nCard)override;

	void doProduceNewBanker();
	void setNextBankerIdx(uint8_t nHuIdx = -1);
	uint8_t getBankerIdx() { return m_nBankerIdx; }
	bool isWaitPlayerActForever() { return true; }
	void onPlayerMo(uint8_t nIdx)override;
	void onPlayerChu(uint8_t nIdx, uint8_t nCard)override;
	void onPlayerChu(uint8_t nIdx, uint8_t nCard, uint8_t& nTing)override;
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
	void onAskForHuThisCard(uint8_t nInvokeIdx, uint8_t nCard, uint8_t& nOutWaitHuIdx, const std::vector<uint8_t>& vAlreadyActIdx);
	void onAskForPengGangThisCard(uint8_t nInvokeIdx, uint8_t nCard, uint8_t& nOutWaitPengIdx, const std::vector<uint8_t>& vAlreadyActIdx);
	void onAskForEatThisCard(uint8_t nInvokeIdx, uint8_t nCard, uint8_t& nOutWaitEatIdx, const std::vector<uint8_t>& vAlreadyActIdx);
	bool isAnyPlayerRobotGang(uint8_t nInvokeIdx, uint8_t nCard)override;
	void onAskForRobotGang(uint8_t nInvokeIdx, uint8_t nCard, std::vector<uint8_t>& vOutCandinates)override;
	uint8_t getNextActPlayerIdx(uint8_t nCurActIdx)override;
	bool isPlayerRootDirectGang(uint8_t nInvokerIdx, uint8_t nCard);
	void onAskForRobotDirectGang(uint8_t nInvokeIdx, uint8_t nActIdx, uint8_t nCard, std::vector<uint8_t>& vOutCandinates);
	void onPlayerTing(uint8_t nIdx, uint8_t nTing, bool bNotCheck = false);
	bool onWaitPlayerGangTing(uint8_t nIdx);
	void onPlayerSingalMo(uint8_t nIdx);

	uint8_t getFanLimit();
	bool isDPOnePay();
	uint8_t getBaseScore();
	uint32_t getGuang();
	bool isEnable7Pair();
	bool isEnableOOT();
	bool isEnableSB1();
	bool isEnableFollow();
	bool isEnableZha5();
	bool isEnableCool();
	bool isEnablePJH();
	bool isCircle();

	void addGain(uint8_t nIdx, stSettleGain stGain);
	void clearGain();
	void backGain(uint8_t nIdx);

	bool canGang() override;
	void onPrePlayerGang();

	bool isHaveCyclone() { return false; }
	bool isOneCircleEnd() { return m_bOneCircleEnd; }
	void signOneCircleEnd() { m_bOneCircleEnd = true; }
	void clearOneCircleEnd() { m_bOneCircleEnd = false; }
protected:
	void addSettle(stSettle& tSettle);
	void settleInfoToJson(Json::Value& jsRealTime, bool& isHuangZhuang);
	void sortFanxing2FanCnt(std::vector<eFanxingType>& vType, uint16_t& nFanCnt);
	void sendStartGameMsg();

protected:
	FXMJPoker m_tPoker;
	std::vector<stSettle> m_vSettle;
	FXMJFanxingChecker m_cFanxingChecker;

	uint8_t m_nNextBankerIdx;
	bool m_bOneCircleEnd;
	std::vector<std::vector<stSettleGain>> m_vGainChip;
	/*uint8_t m_nGangCnt;
	uint8_t m_nDice;
	uint8_t m_nR7;
	uint8_t m_nR15;*/

	bool m_bCheckFollow;
	uint8_t m_nFollowCard;
	uint8_t m_nFollowCnt;

	std::map<uint8_t, uint32_t> m_mSitedIdxes;

};