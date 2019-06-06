#pragma once
#include "GameRoom.h"
#include "IPoker.h"
#include "CardPoker.h"
class NNRoom
	:public GameRoom
{
public:
	enum eResultType
	{
		eReulst_NiuNiu3,
		eResult_NiuNiu5,
		eResult_NiuNiu4 = eResult_NiuNiu5,
		eResult_NiuNiu10,
		eResult_Max,
	};

	enum eDecideBankerType
	{
		eDecideBank_NiuNiu,
		eDecideBank_OneByOne,
		eDecideBank_LookCardThenRobot,
		eDecideBank_Rand,
		eDecideBank_NoNiuLeaveBanker,
		eDecideBank_Max,
	};

public:
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, std::shared_ptr<IGameOpts> ptrGameOpts)override;
	IGamePlayer* createGamePlayer()override;
	void packRoomInfo(Json::Value& jsRoomInfo)override;
	void visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo, uint32_t nVisitorSessionID )override;
	uint8_t getRoomType()override;
	void onStartGame()override;
	void onGameEnd()override;
	bool canStartGame()override;
	IPoker* getPoker()override;
	bool onPlayerNetStateRefreshed(uint32_t nPlayerID, eNetState nState)override;

	void onPlayerReady( uint16_t nIdx);
	uint8_t doProduceNewBanker();
	void doStartBet();
	uint8_t onPlayerDoBet(uint16_t nIdx , uint8_t nBetTimes );
	bool isAllPlayerDoneBet();

	void doDistributeCard( uint8_t nCardCnt );
	uint8_t onPlayerDoCacuateNiu( uint16_t nIdx );
	bool isAllPlayerCacualtedNiu();

	void doStartRobotBanker();
	uint8_t onPlayerRobotBanker( uint16_t nIdx, uint8_t nRobotTimes );
	bool isAllPlayerRobotedBanker();
	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override;
	bool isEnableTuiZhu();
	bool isEnableTuiZhuang();
	void onTimeOutPlayerAutoBet();
	void onTimeOutPlayerAutoRobBanker();
	void invokerTuoGuanAction( uint8_t nTargetIdx = -1 );
	bool isFirstRound();

	bool doPlayerSitDown(stEnterRoomData* pEnterRoomPlayer, uint16_t nIdx)override;
	//void allPlayerAutoReady();
	uint16_t getBankerIdx() { return m_nBankerIdx; }
protected:
	uint8_t getMiniBetTimes();
	int16_t getBeiShuByCardType( uint16_t nType , uint16_t nPoint );
	std::shared_ptr<IPlayerRecorder> createPlayerRecorderPtr()override;
private:
	uint8_t m_eResultType; // 0:Å£Å£x3 Å£¾Åx2 Å£°Ëx2, 1 : Å£Å£x4 Å£¾Åx3 Å£°Ëx2 Å£Æßx2, 2 : Å£Ò» x1 ----Å£Å£x 10
	eDecideBankerType m_eDecideBankerType;
	uint16_t m_nBankerIdx;
	uint16_t m_nBottomTimes;

	uint16_t m_nLastNiuNiuIdx;

	CNiuNiuPoker m_tPoker;
};