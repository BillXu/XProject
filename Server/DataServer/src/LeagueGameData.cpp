#include "LeagueGameData.h"
#include "log4z.h"
#include <algorithm>

CLeagueGameData::CLeagueGameData() {
	m_eType = eLeagueComponent_GameData;
	m_vCreatedRooms.clear();
}

CLeagueGameData::~CLeagueGameData() {

}

void CLeagueGameData::init(CLeague* pLeague) {
	ILeagueComponent::init(pLeague);
}

void CLeagueGameData::reset() {
	m_vCreatedRooms.clear();
}

bool CLeagueGameData::onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort, uint32_t nSenderID) {
	return false;
}

bool CLeagueGameData::onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult) {
	if (eAsync_league_or_club_DeleteRoom == nRequestType) {
		auto nRoomID = jsReqContent["roomID"].asUInt();
		if (isLeagueCreateThisRoom(nRoomID)) {
			auto it = std::find_if(m_vCreatedRooms.begin(), m_vCreatedRooms.end(), [nRoomID](stRoomEntry& str) {
				return str.nRoomID == nRoomID;
			});
			if (it != m_vCreatedRooms.end()) {
				m_vCreatedRooms.erase(it);
			}
		}
		return true;
	}

	if (eAsync_league_CreateRoom == nRequestType) {
		auto nRoomID = jsReqContent["roomID"].asUInt();
		auto nPort = (eMsgPort)(jsReqContent["port"].asUInt());
		auto nUserID = jsReqContent["uid"].asUInt();
		auto nClubID = jsReqContent["clubID"].asUInt();
		if (isLeagueCreateThisRoom(nRoomID)) {
			return true;
		}
		m_vCreatedRooms.push_back(stRoomEntry(nRoomID, nPort));
		LOGFMTD("player uid = %u create league Room id = %u from clubID = %u , add to room list", nUserID, nRoomID, nClubID);
		return true;
	}

	if (eAsync_league_CreateRoom_Info == nRequestType) {
		if (m_vCreatedRooms.size()) {
			jsResult["ret"] = 0;
			Json::Value jsRooms;
			jsRooms = jsReqContent["rooms"];
			auto nClubID = jsReqContent["clubID"].asUInt();
			for (auto &ref : m_vCreatedRooms) {
				Json::Value jsRoom;
				jsRoom["id"] = ref.nRoomID;
				jsRoom["port"] = ref.nSvrPort;
				if (nClubID) {
					jsRoom["clubID"] = nClubID;
				}
				jsRooms[jsRooms.size()] = jsRoom;
			}
			jsResult["rooms"] = jsRooms;
		}
		else {
			jsResult["ret"] = 1;
		}
		return true;
	}
	return false;
}

uint16_t CLeagueGameData::getRoomCnt() {
	return m_vCreatedRooms.size();
}

bool CLeagueGameData::isLeagueCreateThisRoom(uint32_t nRoomID) {
	return std::find_if(m_vCreatedRooms.begin(), m_vCreatedRooms.end(), [nRoomID](stRoomEntry& str) {
		return str.nRoomID == nRoomID;
	}) != m_vCreatedRooms.end();
}