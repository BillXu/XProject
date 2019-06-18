#pragma once
#include "IMJRoom.h"
#include "IMJPoker.h"
#include "YZMJPoker.h"
class YZMJRoom
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
	bool isHaveLouHu()override { return false; }
	void onPlayerLouHu(uint8_t nIdx, uint8_t nInvokerIdx)override;
	bool isGameOver()override;
	bool isRoomOver()override;
	bool canStartGame() override;

	void doProduceNewBanker();
	void setNextBankerIdx(uint8_t nHuIdx = -1);
	uint8_t getBankerIdx() { return m_nBankerIdx; }
	bool isWaitPlayerActForever() { return true; }
	void onPlayerMingGang(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx)override;
	void onPlayerAnGang(uint8_t nIdx, uint8_t nCard)override;
	void onPlayerBuGang(uint8_t nIdx, uint8_t nCard)override;
	void onPlayerHu(std::vector<uint8_t>& vHuIdx, uint8_t nCard, uint8_t nInvokeIdx)override;
	uint8_t getNextActPlayerIdx(uint8_t nCurActIdx)override;
	bool isRoomFull()override { return false; }
	void packStartGameReplyInfo(Json::Value& jsFrameArg)override;

	uint8_t getDa() { return m_nPeiZiCard; }
	void confirmDa();

	bool isPeiZi();
	bool isBaiBanPeiZi();
	bool isYiTiaoLong();
	bool isQiDui();
	uint32_t getGuang();

	bool needChu();

	void doRandomChangeSeat();
	bool doChangeSeat(uint16_t nIdx, uint16_t nWithIdx);
	
	bool isInternalShouldCloseAll();
protected:
	void addSettle(stSettle& tSettle);
	void settleInfoToJson(Json::Value& jsRealTime);
	void sortFanxing2FanCnt(std::vector<eFanxingType>& vType, uint16_t& nFanCnt);
	void sendStartGameMsg();

protected:
	YZMJPoker m_tPoker;
	std::vector<stSettle> m_vSettle;

	uint8_t m_nNextBankerIdx;
	std::vector<std::vector<stSettleGain>> m_vGainChip;
	uint8_t m_nDice;
	uint8_t m_nPeiZiCard;

};