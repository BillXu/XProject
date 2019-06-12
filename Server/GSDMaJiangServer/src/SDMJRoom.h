#pragma once
#include "IMJRoom.h"
#include "SDMJFanxingChecker.h"
#include "SDMJPoker.h"
class SDMJRoom
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
	bool isHaveLouPeng()override { return true; }
	bool isHaveLouHu()override { return true; }
	bool isGameOver()override;
	bool isRoomOver()override;
	//bool isCanGoOnMoPai()override;
	bool canStartGame() override;

	void doProduceNewBanker();
	void doFanBei();
	void doWillFanBei();
	uint8_t getBankerIdx() { return m_nBankerIdx; }
	bool isWaitPlayerActForever() { return true; }
	void onPlayerChu(uint8_t nIdx, uint8_t nCard)override;
	void onPlayerMingGang(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx)override;
	void onPlayerHu(std::vector<uint8_t>& vHuIdx, uint8_t nCard, uint8_t nInvokeIdx)override;
	void onWaitPlayerAct(uint8_t nIdx, bool& isCanPass)override;
	bool isAnyPlayerRobotGang(uint8_t nInvokeIdx, uint8_t nCard)override;
	void onAskForRobotGang(uint8_t nInvokeIdx, uint8_t nCard, std::vector<uint8_t>& vOutCandinates)override;
	uint8_t getNextActPlayerIdx(uint8_t nCurActIdx)override;
	void packStartGameReplyInfo(Json::Value& jsFrameArg)override;

	//uint8_t getFanLimit();
	//bool isDPOnePay();
	//uint8_t getBaseScore();
	uint32_t getGuang();
	bool isDiLing();
	bool isEanableHHQD();
	uint8_t getRuleMode();
	bool isEnableQiDui();
	bool isEnableOnlyZM();

	void addGain(uint8_t nIdx, stSettleGain stGain);
	void clearGain();
	void backGain(uint8_t nIdx);

	//bool canGang() override;
	//void onPrePlayerGang();
	bool needChu();

	void doRandomChangeSeat();
	bool doChangeSeat(uint16_t nIdx, uint16_t nWithIdx);

	void setLastHuIdx(uint8_t nIdx);
	void setYiPaoDuoXiang(uint8_t nIdx);
protected:
	void addSettle(stSettle& tSettle);
	void settleInfoToJson(Json::Value& jsRealTime);
	void sortFanxing2FanCnt(std::vector<eFanxingType>& vType, uint16_t& nFanCnt);
	void sendStartGameMsg();

protected:
	SDMJPoker m_tPoker;
	std::vector<stSettle> m_vSettle;
	SDMJFanxingChecker m_cFanxingChecker;

	std::vector<std::vector<stSettleGain>> m_vGainChip;
	uint8_t m_nDice;

	bool m_bFanBei;
	bool m_bWillFanBei;
	bool m_bBankerHu;

	uint8_t m_nLastHuIdx;
	uint8_t m_nYiPaoDuoXiang;
	bool m_bHuangZhuang;
};