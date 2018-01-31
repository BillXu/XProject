#include "ThirteenGPrivateRoom.h"
#include "Thirteen\ThirteenRoom.h"
#include "stEnterRoomData.h"

ThirteenGPrivateRoom::~ThirteenGPrivateRoom() {
	for (auto& ref : m_vPRooms) {
		if (ref) {
			if (m_pRoom == ref) {
				m_pRoom = nullptr;
			}
			delete ref;
			ref = nullptr;
		}
	}
	m_vPRooms.clear();
}

bool ThirteenGPrivateRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts) {
	if (ThirteenPrivateRoom::init(pRoomMgr, nSeialNum, nRoomID, nSeatCnt, vJsOpts)) {
		m_vPRooms.push_back(m_pRoom);
		m_nMaxCnt = vJsOpts["maxSeatCnt"].asUInt();
		if (m_nMaxCnt < nSeatCnt) {
			m_nMaxCnt = nSeatCnt;
		}
		return true;
	}
	return false;
}

void ThirteenGPrivateRoom::setCurrentPointer(IGameRoom* pRoom) {
	if (std::find(m_vPRooms.begin(), m_vPRooms.end(), pRoom) == m_vPRooms.end()) {
		assert(0 && "invalid argument");
	}
	else {
		if (dynamic_cast<ThirteenRoom*>(pRoom)) {
			m_pRoom = (GameRoom*)pRoom;
		}
		else {
			assert(0 && "invalid argument");
		}
	}
}

bool ThirteenGPrivateRoom::onPlayerEnter(stEnterRoomData* pEnterRoomPlayer)
{
	pEnterRoomPlayer->nChip = 0;
	if (m_vPRooms.empty() || m_pRoom == nullptr)
	{
		LOGFMTE("why room is null ? ");
		return false;
	}

	/*if (m_pRoom->onPlayerEnter(pEnterRoomPlayer))
	{

	}*/
	//TODO
	for (auto& ref : m_vPRooms) {
		if (ref->isRoomFull()) {
			continue;
		}
		if (ref->onPlayerEnter(pEnterRoomPlayer)) {
			return true;
		}
	}

	return true;
}