#pragma once
#include "IGameRoomManager.h"
#define MAX_NOT_SHUFFLE_CARDS_CNT 50
class DDZRoomManager
	:public IGameRoomManager
{
public:
	IGameRoom* createRoom(uint8_t nGameType)override;
	uint16_t getDiamondNeed(uint8_t nGameType, uint8_t nLevel,ePayRoomCardType payType, uint16_t nSeatCnt )override;
	void getNotShuffleCards(std::vector<uint8_t>& vCards);
	void addNotShuffleCards(std::vector<uint8_t> vCards);

protected:
	std::vector<std::vector<uint8_t>> m_vFALNotShuffleCards;
};
