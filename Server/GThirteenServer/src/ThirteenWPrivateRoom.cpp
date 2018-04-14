#include "ThirteenWPrivateRoom.h"
#include "Thirteen\ThirteenRoom.h"
#include "stEnterRoomData.h"

ThirteenWPrivateRoom::~ThirteenWPrivateRoom() {
	for (auto ref : m_mStayPlayers) {
		if (ref.second) {
			delete ref.second;
			ref.second = nullptr;
		}
	}
	m_mStayPlayers.clear();

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

bool ThirteenWPrivateRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts) {
	if (IPrivateRoom::init(pRoomMgr, nSeialNum, nRoomID, nSeatCnt, vJsOpts)) {
		m_isForbitEnterRoomWhenStarted = vJsOpts["forbidJoin"].asUInt() == 1;
		m_nAutoOpenCnt = vJsOpts["starGame"].asUInt();
		m_nMaxCnt = vJsOpts["maxCnt"].isUInt() ? vJsOpts["maxCnt"].asUInt() : 0;

		m_nStartTime = vJsOpts["startTime"].isUInt() ? vJsOpts["startTime"].asUInt() : 0;
		m_bNeedVerify = vJsOpts["needVerify"].isUInt() ? vJsOpts["needVerify"].asBool() : false;
		m_nInitialCoin = vJsOpts["initialCoin"].isUInt() ? vJsOpts["initialCoin"].asUInt() : 0;
		m_nRiseBlindTime = vJsOpts["rbt"].isUInt() ? vJsOpts["rbt"].asUInt() : 0;
		m_nRebuyLevel = vJsOpts["rebuyLevel"].isUInt() ? vJsOpts["rebuyLevel"].asUInt() : 0;
		m_nRebuyTime = vJsOpts["rebuyTime"].isUInt() ? vJsOpts["rebuyTime"].asUInt() : 0;
		m_nEnterFee = vJsOpts["enterFee"].isUInt() ? vJsOpts["enterFee"].asUInt() : 0;
		m_nDelayEnterLevel = vJsOpts["delayEnter"].isUInt() ? vJsOpts["delayEnter"].asUInt() : 0;

		if (vJsOpts["clubID"].isNull() || vJsOpts["clubID"].isUInt() == false) {
			m_nClubID = 0;
		}
		else {
			m_nClubID = vJsOpts["clubID"].asUInt();
		}

		if (vJsOpts["leagueID"].isNull() || vJsOpts["leagueID"].isUInt() == false) {
			m_nLeagueID = 0;
		}
		else {
			m_nLeagueID = vJsOpts["leagueID"].asUInt();
		}

		m_ptrRoomRecorder = getCoreRoom()->createRoomRecorder();
		m_ptrRoomRecorder->init(nSeialNum, nRoomID, getCoreRoom()->getRoomType(), vJsOpts["uid"].asUInt(), vJsOpts);
		m_ptrRoomRecorder->setClubID(m_nClubID);
		m_ptrRoomRecorder->setLeagueID(m_nLeagueID);

		((ThirteenRoom*)m_pRoom)->signIsWaiting();
		m_vPRooms.push_back(m_pRoom);
		return true;
	}
	return false;
}

void ThirteenWPrivateRoom::setCurrentPointer(IGameRoom* pRoom) {
	if (std::find(m_vPRooms.begin(), m_vPRooms.end(), pRoom) == m_vPRooms.end()) {
		//assert(0 && "invalid argument");
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

void ThirteenWPrivateRoom::packRoomInfo(Json::Value& jsRoomInfo) {
	IPrivateRoom::packRoomInfo(jsRoomInfo);
}

bool ThirteenWPrivateRoom::onPlayerEnter(stEnterRoomData* pEnterRoomPlayer) {
	if (isRoomStarted()) {
		auto sPlayer = isEnterByUserID(pEnterRoomPlayer->nUserUID);
		if (sPlayer) {
			sPlayer->nSessionID = pEnterRoomPlayer->nSessionID;
			if ((uint32_t)-1 == sPlayer->nCurInIdx) {
				if (sPlayer->nChip) {
					packTempRoomInfoToPlayer(pEnterRoomPlayer);
					return false;
				}
				else {
					//观战TODO
					return enterRoomToWatch(pEnterRoomPlayer);
				}
			}
			else {
				if (sPlayer->nCurInIdx < m_vPRooms.size()) {
					m_pRoom = m_vPRooms[sPlayer->nCurInIdx];
					if (m_pRoom->onPlayerEnter(pEnterRoomPlayer)) {

					}
					return true;
				}
				else {
					sPlayer->nCurInIdx = -1;
					if (sPlayer->nChip) {
						packTempRoomInfoToPlayer(pEnterRoomPlayer);
						return false;
					}
					else {
						//观战TODO
						return enterRoomToWatch(pEnterRoomPlayer);
					}
				}
			}
		}
		else {
			
		}
	}
	return false;
	//return IPrivateRoom::onPlayerEnter(pEnterRoomPlayer);
}

uint8_t ThirteenWPrivateRoom::checkPlayerCanEnter(stEnterRoomData* pEnterRoomPlayer) {
	auto sPlayer = isEnterByUserID(pEnterRoomPlayer->nUserUID);
	if (sPlayer) {
		return ThirteenGPrivateRoom::checkPlayerCanEnter(pEnterRoomPlayer);
	}
	return 12;
}

bool ThirteenWPrivateRoom::isRoomFull() {
	return false;
}