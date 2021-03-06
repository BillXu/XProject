#pragma once
#include "GameRoom.h"
#include "IPoker.h"
#include "BJPoker.h"
#include "BJDefine.h"
class BJRoom
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

	void onPlayerReady(uint16_t nIdx);

	bool isAllPlayerMakedGroupCard();
	uint8_t onPlayerDoMakeCardGroup( uint8_t nIdx,std::vector<uint8_t>& vGroupCards );
	bool onPlayerAutoMakeCardGroupAllPlayerOk();

	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override;
protected:
	std::shared_ptr<IPlayerRecorder> createPlayerRecorderPtr()override;
	uint8_t getRoomRate() { return m_jsOpts["times"].asUInt(); }
	bool isEnableSanQing() { return m_jsOpts["isSQ"].asUInt() == 1; }
	bool isEnableShunQingDaTou() { return m_jsOpts["isShunqing"].asUInt() == 1; }
public:
	bool isEnableGiveUp() { return m_jsOpts["isGiveUp"].asUInt() == 1; }
private:
	CBJPoker m_tPoker;
};