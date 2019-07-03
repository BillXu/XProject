#pragma once
#include "IMJRoom.h"
#include "NJMJFanxingChecker.h"
#include "NJMJPoker.h"
class NJMJRoom
	:public IMJRoom
{
public:
	struct stSettle
	{
		std::map<uint8_t, uint16_t> vWinIdxs;
		std::map<uint8_t, uint16_t> vLoseIdx;
		eMJActType eSettleReason;
		Json::Value jsHuMsg;
		bool bWaiBao = false;
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

	struct stFollow
	{
	public:
		uint8_t m_nCard = 0;
		uint8_t m_nFirstIdx = -1;
		uint8_t m_nSeatCnt = 0;
		std::vector<uint8_t> m_vIdxes;

		void setSeatCnt(uint8_t nSeatCnt) {
			m_nSeatCnt = nSeatCnt;
			reset();
		}

		void reset() {
			m_nCard = 0;
			m_nFirstIdx = -1;
			m_vIdxes.clear();
		}

		bool onChuCard(uint8_t nIdx, uint8_t nCard) {
			if (nCard == 0) {
				reset();
				return false;
			}

			if (m_nSeatCnt < 4) {
				return false;
			}

			if ((uint8_t)-1 == m_nFirstIdx) {
				reset();
			}

			if (nCard == m_nCard) {
				if (std::find(m_vIdxes.begin(), m_vIdxes.end(), nIdx) == m_vIdxes.end()) {
					m_vIdxes.push_back(nIdx);
					if (m_vIdxes.size() == m_nSeatCnt) {
						return true;
					}
				}
			}

			reset();
			m_nCard = nCard;
			m_nFirstIdx = nIdx;
			m_vIdxes.push_back(nIdx);
			return false;
		}
	};

	struct stCheckHuCard
	{
	public:
		uint8_t m_nCard = 0;
		uint8_t m_nIdx = -1;
		eMJActType m_nActType = eMJAct_None;

		void reset() {
			m_nCard = 0;
			m_nIdx = -1;
			m_nActType = eMJAct_None;
		}

		void setHuCard(uint8_t nCard, uint8_t nIdx, eMJActType nActType) {
			m_nCard = nCard;
			m_nIdx = nIdx;
			m_nActType = nActType;
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
	bool canStartGame() override;

	uint8_t getBankerIdx() { return m_nBankerIdx; }
	bool isWaitPlayerActForever() { return true; }
	void onPlayerChu(uint8_t nIdx, uint8_t nCard)override;
	void onPlayerMingGang(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx)override;
	void onPlayerAnGang(uint8_t nIdx, uint8_t nCard)override;
	void onPlayerBuGang(uint8_t nIdx, uint8_t nCard)override;
	void onPlayerHu(std::vector<uint8_t>& vHuIdx, uint8_t nCard, uint8_t nInvokeIdx)override;
	void onWaitPlayerAct(uint8_t nIdx, bool& isCanPass)override;
	bool isAnyPlayerRobotGang(uint8_t nInvokeIdx, uint8_t nCard)override;
	bool isAnyPlayerPengOrHuThisCard(uint8_t nInvokeIdx, uint8_t nCard)override;
	uint8_t getNextActPlayerIdx(uint8_t nCurActIdx)override;
	void packStartGameReplyInfo(Json::Value& jsFrameArg)override;
	bool isOneCircleEnd()override;

	uint8_t getBaseScore()override;
	uint32_t getGuang();
	bool isEnableJieZhuangBi();
	bool isEnableHuaZa();
	bool isEnableSiLianFeng();
	bool isEnableWaiBao();
	bool isEnableBiXiaHu();
	bool isEnableLeiBaTa();
	bool isEnableShuangGang();
	bool isEnableYiDuiDaoDi();

	bool needChu();
	void signOneCircleEnd() { m_bOneCircleEnd = true; }
	void clearOneCircleEnd() { m_bOneCircleEnd = false; }

	bool isInternalShouldCloseAll();
	void doRandomChangeSeat();
	bool doChangeSeat(uint16_t nIdx, uint16_t nWithIdx);

	bool isFanBei() { return isEnableBiXiaHu() && m_bFanBei; }
	uint32_t getLBTCnt() { return m_nLeiBaTaCnt; }
	bool checkBaoGuang(uint8_t& nIdx);
	bool checkHuGuang(uint8_t& nIdx);
protected:
	void addSettle(stSettle& tSettle);
	void settleInfoToJson(Json::Value& jsRealTime);
	void sortFanxing2FanCnt(std::vector<eFanxingType>& vType, uint16_t& nFanCnt);
	void sendStartGameMsg();

	void doProduceNewBanker();
	void setNextBankerIdx(std::vector<uint8_t>& vHuIdx);
	void doWillFanBei();
	void doLeiBaTa();

protected:
	NJMJPoker m_tPoker;
	std::vector<stSettle> m_vSettle;
	NJMJFanxingChecker m_cFanxingChecker;

	uint8_t m_nNextBankerIdx;
	bool m_bOneCircleEnd;
	bool m_bWillFanBei;
	bool m_bFanBei;
	uint8_t m_nLeiBaTaCnt;
	uint8_t m_nGangCnt;

	stFollow m_cFollowCards;
	stCheckHuCard m_cCheckHuCard;
};