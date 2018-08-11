#pragma once
#include "ICoinRoom.h"
class RedBlackCoinRoom
	:public ICoinRoom
{
public:
	void onStartGame(IGameRoom* pRoom)override;
	bool onPlayerNetStateRefreshed(uint32_t nPlayerID, eNetState nState)override;
	GameRoom* doCreatRealRoom()override;
	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID) override;
	void onPlayerDoLeaved(IGameRoom* pRoom, uint32_t nUserUID)override;
	void sendRoomInfo(uint32_t nSessionID)override;
};