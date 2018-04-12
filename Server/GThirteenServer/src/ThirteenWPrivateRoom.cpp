#include "ThirteenWPrivateRoom.h"
#include "Thirteen\ThirteenRoom.h"

ThirteenWPrivateRoom::~ThirteenWPrivateRoom() {

}

bool ThirteenWPrivateRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts) {
	if (ThirteenGPrivateRoom::init(pRoomMgr, nSeialNum, nRoomID, nSeatCnt, vJsOpts)) {
		
		return true;
	}
	return false;
}

void ThirteenWPrivateRoom::packRoomInfo(Json::Value& jsRoomInfo) {
	IPrivateRoom::packRoomInfo(jsRoomInfo);
	/*if (m_nOverType == ROOM_OVER_TYPE_TIME) {
		jsRoomInfo["leftTime"] = (int32_t)m_tCreateTimeLimit.getDuringTime();
		jsRoomInfo["time"] = (int32_t)m_tCreateTimeLimit.getInterval();
	}*/
	//jsRoomInfo["RBPool"] = m_nRotBankerPool;
	//jsRoomInfo["RBPool"] = 0;
}