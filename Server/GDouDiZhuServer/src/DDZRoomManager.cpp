#include "DDZRoomManager.h"
#include "DDZPrivateRoom.h"
IGameRoom* DDZRoomManager::createRoom(uint8_t nGameType)
{
	if ( eGame_CYDouDiZhu == nGameType || eGame_JJDouDiZhu == nGameType )
	{
		return new DDZPrivateRoom();
	}
	LOGFMTE("unknown game type = %u , can not create private room", nGameType );
	return nullptr;
}

//uint16_t DDZRoomManager::getDiamondNeed(uint8_t nGameType, uint8_t nLevel, ePayRoomCardType payType, uint16_t nSeatCnt )
//{
////#ifdef _DEBUG
////	return 0;
////#endif // _DEBUG
//
//	if (isCreateRoomFree())
//	{
//		return 0;
//	}
//
//	if (nLevel >= 5)
//	{
//		LOGFMTE( "invalid room level for game = %u , level = %u",nGameType,nLevel );
//		nLevel = 0;
//	}
//
//	// is aa true ;
//	if (ePayType_AA == payType)
//	{
//		uint16_t vAA[] = { 1 , 2 , 3 , 1 , 2 };
//		if (nLevel < 3) {
//			return vAA[nLevel] * 10;
//		}
//		return vAA[nLevel];
//	}
//
//	// 6,1 . 12.2 , 18. 3
//	uint16_t vFangZhu[] = { 3 , 6 , 9 , 3 , 6 };
//	if (nLevel < 3) {
//		return vFangZhu[nLevel] * 10;
//	}
//	return vFangZhu[nLevel];
//}

void DDZRoomManager::addNotShuffleCards(std::vector<uint8_t> vCards) {
	if (vCards.size() == 54) {
		if (m_vFALNotShuffleCards.size() < MAX_NOT_SHUFFLE_CARDS_CNT) {
			m_vFALNotShuffleCards.push_back(vCards);
		}
		else {
			uint8_t i = rand() % MAX_NOT_SHUFFLE_CARDS_CNT;
			m_vFALNotShuffleCards[i] = vCards;
		}
	}
}

void DDZRoomManager::getNotShuffleCards(std::vector<uint8_t>& vCards) {
	vCards.clear();
	uint8_t nSize = m_vFALNotShuffleCards.size();
	if (nSize) {
		uint8_t i = rand() % nSize;
		vCards.assign(m_vFALNotShuffleCards[i].begin(), m_vFALNotShuffleCards[i].end());
		m_vFALNotShuffleCards.erase(m_vFALNotShuffleCards.begin() + i);
	}
}