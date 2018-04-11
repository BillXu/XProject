#include "ThirteenWPrivateRoom.h"

ThirteenWPrivateRoom::~ThirteenWPrivateRoom() {

}

bool ThirteenWPrivateRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts) {
	if (ThirteenGPrivateRoom::init(pRoomMgr, nSeialNum, nRoomID, nSeatCnt, vJsOpts)) {
		
		return true;
	}
	return false;
}