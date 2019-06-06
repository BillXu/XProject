#pragma once
#include "GameRoom.h"
#include "IPoker.h"
#include "CardPoker.h"
#include "DouDiZhuDefine.h"
class DDZRoom
	:public GameRoom
{
public:
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, std::shared_ptr<IGameOpts> ptrGameOpts)override;
	IGamePlayer* createGamePlayer()override;
	void packRoomInfo(Json::Value& jsRoomInfo)override;
	void visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo, uint32_t nVisitorSessionID)override;
	uint8_t getRoomType()override;
	void onWillStartGame()override;
	void onStartGame()override;
	void onGameEnd()override;
	bool canStartGame()override;
	IPoker* getPoker()override;
	bool isRoomFull()override { return false; }
	uint8_t checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer)override { return 0; }
	uint8_t checkPlayerCanSitDown(stEnterRoomData* pEnterRoomPlayer)override;

	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override;
	uint8_t getFirstRobotBankerIdx();
	bool setFirstRotBankerIdx(uint8_t nIdx);
	uint8_t getBankerIdx() { return m_nBankerIdx; }
	void setNewBankerInfo( uint8_t nBankerIdx , uint8_t nBankerTimes , std::vector<uint8_t>& vDiPai );
	uint8_t getBankTimes();
	uint8_t getBombCount();
	void increaseBombCount();
	uint32_t fengDing();
	DDZ_RotLandlordType getRLT();
	bool isCYDDZ();
	bool isCanDouble();
	uint8_t getBaseScore();
	uint32_t getFanLimit();
	bool isChaoZhuang();
	bool isNotShuffle();
	void addNotShuffleCards(std::vector<uint8_t>& vCards);
	void getNotShuffleCards(std::vector<uint8_t>& vCards);
	void initCardsWithNotShuffleCards();
private:
	uint8_t m_nFirstRobotBankerIdx;
	uint8_t m_nBankerIdx;
	uint8_t m_nBankerTimes;
	CDouDiZhuPoker m_tPoker;
	std::vector<uint8_t> m_vDiPai;
	uint8_t m_nBombCnt;

	std::vector<uint8_t> m_vNotShuffleCards;
};