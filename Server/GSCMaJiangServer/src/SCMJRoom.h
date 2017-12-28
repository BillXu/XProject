#pragma once
#include "IMJRoom.h"
#include "IMJPoker.h"
#include "SCMJFanxingChecker.h"
class SCMJRoom
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
			vWinIdxs[nIdx] = nWinCoin;
		}

		void addLose(uint8_t nIdx, uint16_t nLoseCoin)
		{
			vLoseIdx[nIdx] = nLoseCoin;
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

	void doProduceNewBanker();
	uint8_t getBankerIdx() { return m_nBankerIdx; }
	bool isWaitPlayerActForever() { return true; }
	void onPlayerMo(uint8_t nIdx)override;
	void onPlayerChu(uint8_t nIdx, uint8_t nCard)override;
	void onPlayerPeng(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx)override;
	void onPlayerMingGang(uint8_t nIdx, uint8_t nCard, uint8_t nInvokeIdx)override;
	void onPlayerAnGang(uint8_t nIdx, uint8_t nCard)override;
	void onPlayerBuGang(uint8_t nIdx, uint8_t nCard)override;
	void onPlayerHu(std::vector<uint8_t>& vHuIdx, uint8_t nCard, uint8_t nInvokeIdx)override;
	void onWaitPlayerAct(uint8_t nIdx, bool& isCanPass)override;
	void onAskForPengOrHuThisCard(uint8_t nInvokeIdx, uint8_t nCard, std::vector<uint16_t>& vOutWaitHuIdx, std::vector<uint16_t>& vOutWaitPengGangIdx, bool& isNeedWaitEat)override;
	bool isAnyPlayerRobotGang(uint8_t nInvokeIdx, uint8_t nCard)override;
	void onAskForRobotGang(uint8_t nInvokeIdx, uint8_t nCard, std::vector<uint8_t>& vOutCandinates)override;
	uint8_t getNextActPlayerIdx(uint8_t nCurActIdx)override;
	bool onWaitPlayerExchangeCards();
	void onPlayerExchangeCards(std::map<uint8_t, std::vector<uint8_t>>& exChangeCards);
	void onAutoDecidePlayerExchangeCards(std::map<uint8_t, std::vector<uint8_t>>& exChangeCards);
	void onPlayerDecideExchangeCards(uint8_t idx, std::vector<uint8_t> vCards, std::map<uint8_t, std::vector<uint8_t>>& exChangeCards);
	bool isAllPlayerConfirmMiss();
	void onShowPlayerMiss();
	bool onWaitPlayerConfirmMiss();
	void onPlayerConfirmMiss(uint8_t idx, uint8_t nCardType);
	void onAutoConfirmMiss();

	bool isEnableBloodFlow();
	bool isZiMoAddFan();
	bool isDianGangZiMo();
	bool isEnableExchange3Cards();
	bool isEnable19();
	bool isEnableMenQing();
	bool isEnableZZ();
	bool isEnableTDHu();
	uint8_t getFanLimit();

protected:
	void addSettle(stSettle& tSettle);
	void eraseLastGangSettle();
	void settleInfoToJson(Json::Value& jsRealTime);
	uint8_t getBaseScore() { return 1; }
	void onPlayerChaJiao(Json::Value& jsReal);
	void onPlayerHuaZhu(Json::Value& jsReal);
	void sortFanxing2FanCnt(std::vector<eFanxingType>& vType, uint16_t& nFanCnt);
	uint32_t getMaxChaJiaoScore(uint8_t idx, uint8_t nInvokeIdx);
	void sendStartGameMsg();

protected:
	IMJPoker m_tPoker;
	std::vector<stSettle> m_vSettle;
	SCMJFanxingChecker m_cFanxingChecker;

	uint8_t m_nNextBankerIdx;
	bool m_bIsShowMiss;

};