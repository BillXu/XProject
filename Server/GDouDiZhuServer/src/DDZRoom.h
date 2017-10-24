#pragma once
#include "GameRoom.h"
#include "IPoker.h"
#include "CardPoker.h"
class DDZRoom
	:public GameRoom
{
public:
	bool init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)override;
	IGamePlayer* createGamePlayer()override;
	void packRoomInfo(Json::Value& jsRoomInfo)override;
	void visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo, uint32_t nVisitorSessionID)override;
	uint8_t getRoomType()override;
	void onStartGame()override;
	void onGameEnd()override;
	bool canStartGame()override;
	IPoker* getPoker()override;

	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override;
	uint8_t getFirstRobotBankerIdx();
	uint8_t getBankerIdx() { return m_nBankerIdx; }
	void setNewBankerInfo( uint8_t nBankerIdx , uint8_t nBankerTimes , std::vector<uint8_t>& vDiPai );
	uint8_t getBankTimes();
	uint8_t getBombCount();
	void increaseBombCount();
protected:
	bool addPlayerOneRoundOffsetToRecorder(IGamePlayer* pPlayer)override;
private:
	uint8_t m_nFirstRobotBankerIdx;
	uint8_t m_nBankerIdx;
	uint8_t m_nBankerTimes;
	CDouDiZhuPoker m_tPoker;
	std::vector<uint8_t> m_vDiPai;
	uint8_t m_nBombCnt;
};