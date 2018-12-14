#pragma once
#include "IMJRoom.h"
#include "IMJPoker.h"
class AHMJRoom
	:public IMJRoom
{
public:
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
	uint8_t getRoomType()override;
	IPoker* getPoker()override;
	bool isHaveRace()override;
	void onWaitRace(uint8_t nIdx = -1)override;
	void onAskForPengOrHuThisCard(uint8_t nInvokeIdx, uint8_t nCard, std::vector<uint16_t>& vOutWaitHuIdx, std::vector<uint16_t>& vOutWaitPengGangIdx, bool& isNeedWaitEat)override;
	void onAskForRobotGang(uint8_t nInvokeIdx, uint8_t nCard, std::vector<uint8_t>& vOutCandinates)override;
	bool isWaitPlayerActForever() { return true; }
	bool needChu();
	bool canGang()override;
	void packRoomInfo(Json::Value& jsRoomInfo)override;
	void visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo, uint32_t nVisitorSessionID)override;
	void onWillStartGame()override;
	void onStartGame()override;
	void onGameEnd()override;
	bool isCanGoOnMoPai()override;
	void onPlayerMo(uint8_t nIdx)override;
	void onPlayerChu(uint8_t nIdx, uint8_t nCard)override;
	void onPlayerPeng(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx)override;
	void onPlayerMingGang(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx)override;
	void onPlayerAnGang(uint8_t nIdx, uint8_t nCard)override;
	void onPlayerBuGang(uint8_t nIdx, uint8_t nCard)override;
	void onPlayerHu(std::vector<uint8_t>& vHuIdx, uint8_t nCard, uint8_t nInvokeIdx)override;
	bool isHaveLouHu()override { return false; }
	void onPlayerEat(uint8_t nIdx, uint8_t nCard, uint8_t nWithA, uint8_t nWithB, uint8_t nInvokeIdx)override;
	void onPlayerLouHu(uint8_t nIdx, uint8_t nInvokerIdx)override;
	bool onWaitPlayerActAfterCP(uint8_t nIdx);
	bool isGameOver()override;

	bool isOneCircleEnd() { return m_bOneCircleEnd; }
	void signOneCircleEnd() { m_bOneCircleEnd = true; }
	void clearOneCircleEnd() { m_bOneCircleEnd = false; }

	uint8_t getBankerIdx() { return m_nBankerIdx; }

	uint8_t getBaseScore();
	uint8_t getFanLimit();
	bool isCircle();

protected:
	void doProduceNewBanker();
	void setNextBankerIdx(uint8_t nHuIdx = -1);
	void sendStartGameMsg();
	void sendWillStartGameMsg();

	void addSettle(stSettle& tSettle);
	void settleInfoToJson(Json::Value& jsRealTime, bool& bHuangZhuang);

protected:
	IMJPoker m_tPoker;
	std::vector<stSettle> m_vSettle;
	bool m_bOneCircleEnd;
	uint8_t m_nNextBankerIdx;
};