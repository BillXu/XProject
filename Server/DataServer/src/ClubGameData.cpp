#include "ClubGameData.h"
#include "Club.h"
#include "ClubManager.h"
#include "ClubMemberData.h"
#include "log4z.h"
#include "ISeverApp.h"
#include "AsyncRequestQuene.h"
#include <algorithm>

CClubGameData::CClubGameData() {
	m_eType = eClubComponent_GameData;
	m_vCreatedRooms.clear();
}

CClubGameData::~CClubGameData() {

}

void CClubGameData::init(CClub* pClub) {
	IClubComponent::init(pClub);
}

void CClubGameData::reset() {
	m_vCreatedRooms.clear();
}

bool CClubGameData::onMsg(Json::Value& recvValue, uint16_t nmsgType, eMsgPort eSenderPort, uint32_t nSenderID) {
	if (MSG_CLUB_APPLY_ROOM_INFO == nmsgType) {
		Json::Value jsRooms;
		for (auto& ref : m_vCreatedRooms) {
			Json::Value jsRoomEntry;
			jsRoomEntry["id"] = ref.nRoomID;
			jsRoomEntry["port"] = ref.nSvrPort;
			jsRooms[jsRooms.size()] = jsRoomEntry;
		}
		/*jsInfo["ret"] = 0;
		jsInfo["clubID"] = getClub()->getClubID();
		jsInfo["rooms"] = jsRooms;
		sendMsgToClient(jsInfo, nmsgType, nSenderID);*/

		std::vector<uint32_t> vLeagueIDs;
		vLeagueIDs.assign(getClub()->getBaseData()->vJoinedLeague.begin(), getClub()->getBaseData()->vJoinedLeague.end());
		vLeagueIDs.insert(vLeagueIDs.end(), getClub()->getBaseData()->vCreatedLeague.begin(), getClub()->getBaseData()->vCreatedLeague.end());

		getLeagueRoomInfo(jsRooms, vLeagueIDs, nSenderID);
		return true;
	}
	return false;
}

bool CClubGameData::onAsyncRequest(uint16_t nRequestType, const Json::Value& jsReqContent, Json::Value& jsResult) {
	if (eAsync_league_or_club_DeleteRoom == nRequestType) {
		auto nRoomID = jsReqContent["roomID"].asUInt();
		if (isClubCreateThisRoom(nRoomID)) {
			auto it = std::find_if(m_vCreatedRooms.begin(), m_vCreatedRooms.end(), [nRoomID](stRoomEntry& str) {
				return str.nRoomID == nRoomID;
			});
			if (it != m_vCreatedRooms.end()) {
				m_vCreatedRooms.erase(it);
			}
		}
		return true;
	}

	if (eAsync_Club_CreateRoom == nRequestType) {
		auto nRoomID = jsReqContent["roomID"].asUInt();
		auto nPort = (eMsgPort)(jsReqContent["port"].asUInt());
		auto nUserID = jsReqContent["targetUID"].asUInt();
		auto nLeagueID = jsReqContent["leagueID"].asUInt();
		if (nLeagueID) {
			Json::Value jsInformCreatRoom;
			jsInformCreatRoom["clubID"] = getClub()->getClubID();
			jsInformCreatRoom["uid"] = nUserID;
			jsInformCreatRoom["roomID"] = nRoomID;
			jsInformCreatRoom["port"] = nPort;
			jsInformCreatRoom["leagueID"] = nLeagueID;
			getClub()->getClubMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, nLeagueID, eAsync_league_CreateRoom, jsInformCreatRoom);
		}
		else {
			if (isClubCreateThisRoom(nRoomID)) {
				return true;
			}
			m_vCreatedRooms.push_back(stRoomEntry(nRoomID, nPort));
			LOGFMTD("player uid = %u create club Room id = %u , add to room list", nUserID, nRoomID);
		}
		return true;
	}

	if (eAsync_Club_CreateRoom_Check == nRequestType) {
		if (getClub()->getCreateFlag() == 0) {
			//LOGFMTE("User apply create room for club error, level is error, levelRequire = %u, uid = %u, clubID = %u", nLevelRequire, nUserID, getClub()->getClubID());
			jsResult["ret"] = 2;
			return true;
		}
		auto nLeagueID = jsReqContent["leagueID"].asUInt();
		auto nUserID = jsReqContent["uid"].asUInt();
		auto nLevelRequire = getCreateRoomLevelRequire(getClub()->getCreateRoomType());
		if (nLevelRequire == 0 || getClub()->getClubMemberData()->checkUpdateLevel(nUserID, nLevelRequire) == false) {
			LOGFMTE("User apply create room for club error, level is error, levelRequire = %u, uid = %u, clubID = %u", nLevelRequire, nUserID, getClub()->getClubID());
			jsResult["ret"] = 1;
			return true;
		}
		if (nLeagueID) {
			return false;
		}
		else {
			jsResult["ret"] = 0;
		}
		return true;
	}
	return false;
}

bool CClubGameData::onAsyncRequestDelayResp(uint16_t nRequestType, uint32_t nReqSerial, const Json::Value& jsReqContent, uint16_t nSenderPort, uint32_t nSenderID) {
	if (eAsync_Club_CreateRoom_Check == nRequestType) {
		auto nLeagueID = jsReqContent["leagueID"].asUInt();
		if (nLeagueID == 0) {
			return false;
		}
		else {
			auto nUserID = jsReqContent["uid"].asUInt();
			//auto pAsync = getClub()->getClubMgr()->getSvrApp()->getAsynReqQueue();
			auto pApp = getClub()->getClubMgr()->getSvrApp();
			Json::Value jsReq;
			jsReq["leagueID"] = nLeagueID;
			jsReq["clubID"] = getClub()->getClubID();
			//getClub()->getClubMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, nLeagueID, eAsync_league_CreateRoom_Check, );
			pApp->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, nLeagueID, eAsync_league_CreateRoom_Check, jsReq, [pApp, nSenderID, nReqSerial, nSenderPort, this, nLeagueID, nUserID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
			{
				Json::Value jsRet;
				if (isTimeOut)
				{
					LOGFMTE(" request of league time out uid = %u , can not create room ", nUserID);
					jsRet["ret"] = 7;
					pApp->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsRet, getClub()->getClubID());
					return;
				}

				uint8_t nReqRet = retContent["ret"].asUInt();
				uint8_t nRet = 0;
				do {
					if (0 != nReqRet)
					{
						nRet = 3;
						break;
					}


				} while (0);

				jsRet["ret"] = nRet;
				pApp->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsRet, getClub()->getClubID());
			});
		}
		return true;
	}

	if (eAsync_club_CreateRoom_Info == nRequestType) {
		Json::Value jsRooms;
		jsRooms = jsReqContent["rooms"];
		for (auto& ref : m_vCreatedRooms) {
			Json::Value jsRoomEntry;
			jsRoomEntry["id"] = ref.nRoomID;
			jsRoomEntry["port"] = ref.nSvrPort;
			jsRoomEntry["clubID"] = getClub()->getClubID();
			jsRooms[jsRooms.size()] = jsRoomEntry;
		}
		/*jsInfo["ret"] = 0;
		jsInfo["clubID"] = getClub()->getClubID();
		jsInfo["rooms"] = jsRooms;
		sendMsgToClient(jsInfo, nmsgType, nSenderID);*/

		std::vector<uint32_t> vLeagueIDs;
		vLeagueIDs.assign(getClub()->getBaseData()->vJoinedLeague.begin(), getClub()->getBaseData()->vJoinedLeague.end());
		vLeagueIDs.insert(vLeagueIDs.end(), getClub()->getBaseData()->vCreatedLeague.begin(), getClub()->getBaseData()->vCreatedLeague.end());

		getLeagueRoomInfoByPlayer(jsRooms, vLeagueIDs, nRequestType, nReqSerial, nSenderPort, nSenderID);
		return true;
	}
	return false;
}

uint16_t CClubGameData::getRoomCnt() {
	return m_vCreatedRooms.size();
}

uint8_t CClubGameData::getCreateRoomLevelRequire(uint8_t nCreateType) {
	switch (nCreateType)
	{
	case eClubCreateRoom_All:
	{
		return eClubMemberLevel_None;
	}
	case eClubCreateRoom_Admin:
	{
		return eClubMemberLevel_Admin;
	}
	case eClubCreateRoom_Creator:
	{
		return eClubMemberLevel_Creator;
	}
	default:
		LOGFMTE("Create room level require parse create room type error, type = %u", nCreateType);
		return 0;
	}
}

void CClubGameData::getLeagueRoomInfo(Json::Value& jsRoomInfo, std::vector<uint32_t> vLeagues, uint32_t nSenderID, uint32_t nIdx) {
	//LOGFMTE("Enter request club roomInfo from league, leaguesize = %u", vLeagues.size());
	if (nIdx < vLeagues.size()) {
		Json::Value jsReq;
		auto nLeagueID = vLeagues[nIdx];
		//LOGFMTE("Enter request club roomInfo from league, leagueID = %u", nLeagueID);
		jsReq["leagueID"] = nLeagueID;
		jsReq["rooms"] = jsRoomInfo;
		getClub()->getClubMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, nLeagueID, eAsync_league_CreateRoom_Info, jsReq, [this, nIdx, vLeagues, jsRoomInfo, nSenderID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut) {
			if (isTimeOut) {
				Json::Value jsInfo;
				jsInfo["ret"] = 1;
				jsInfo["clubID"] = getClub()->getClubID();
				jsInfo["rooms"] = jsRoomInfo;
				sendMsgToClient(jsInfo, MSG_CLUB_APPLY_ROOM_INFO, nSenderID);
			}
			else {
				uint8_t nRet = retContent["ret"].asUInt();
				//LOGFMTE("Enter request club roomInfo get response form league, leagueID = %u, ret = %u", vLeagues[nIdx], nRet);
				Json::Value jsRooms;
				if (nRet) {
					jsRooms = jsRoomInfo;
				}
				else {
					jsRooms = retContent["rooms"];
				}
				uint32_t nCurIdx = nIdx + 1;
				if (nCurIdx < vLeagues.size()) {
					//LOGFMTE("Enter request club roomInfo continue read league info from, leagueID = %u", vLeagues[nCurIdx]);
					getLeagueRoomInfo(jsRooms, vLeagues, nSenderID, nCurIdx);
				}
				else {
					//LOGFMTE("Enter request club roomInfo end");
					Json::Value jsInfo;
					jsInfo["ret"] = 0;
					jsInfo["clubID"] = getClub()->getClubID();
					jsInfo["rooms"] = jsRooms;
					sendMsgToClient(jsInfo, MSG_CLUB_APPLY_ROOM_INFO, nSenderID);
				}
			}
			
		});
	}
	else if (vLeagues.size() == 0) {
		//LOGFMTE("Enter request club roomInfo end with empty");
		Json::Value jsInfo;
		jsInfo["ret"] = 0;
		jsInfo["clubID"] = getClub()->getClubID();
		jsInfo["rooms"] = jsRoomInfo;
		sendMsgToClient(jsInfo, MSG_CLUB_APPLY_ROOM_INFO, nSenderID);
	}
}

void CClubGameData::getLeagueRoomInfoByPlayer(Json::Value& jsRoomInfo, std::vector<uint32_t> vLeagues, uint16_t nRequestType, uint32_t nReqSerial, uint16_t nSenderPort, uint32_t nSenderID, uint32_t nIdx) {
	if (nIdx < vLeagues.size()) {
		Json::Value jsReq;
		auto nLeagueID = vLeagues[nIdx];
		jsReq["leagueID"] = nLeagueID;
		jsReq["clubID"] = getClub()->getClubID();
		jsReq["rooms"] = jsRoomInfo;
		getClub()->getClubMgr()->getSvrApp()->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, nLeagueID, eAsync_league_CreateRoom_Info, jsReq, [this, nIdx, vLeagues, jsRoomInfo, nRequestType, nReqSerial, nSenderPort, nSenderID](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut) {
			if (isTimeOut) {
				Json::Value jsRet;
				jsRet["ret"] = 0;
				jsRet["rooms"] = jsRoomInfo;
				getClub()->getClubMgr()->getSvrApp()->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsRet, getClub()->getClubID());


				/*Json::Value jsInfo;
				jsInfo["ret"] = 1;
				jsInfo["clubID"] = getClub()->getClubID();
				jsInfo["rooms"] = jsRoomInfo;
				sendMsgToClient(jsInfo, MSG_CLUB_APPLY_ROOM_INFO, nSenderID);*/
			}
			else {
				uint8_t nRet = retContent["ret"].asUInt();
				Json::Value jsRooms;
				if (nRet) {
					jsRooms = jsRoomInfo;
				}
				else {
					jsRooms = retContent["rooms"];
				}
				uint32_t nCurIdx = nIdx + 1;
				if (nCurIdx < vLeagues.size()) {
					getLeagueRoomInfoByPlayer(jsRooms, vLeagues, nRequestType, nReqSerial, nSenderPort, nSenderID, nCurIdx);
				}
				else {
					Json::Value jsRet;
					jsRet["ret"] = 0;
					jsRet["rooms"] = jsRooms;
					getClub()->getClubMgr()->getSvrApp()->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsRet, getClub()->getClubID());

					/*Json::Value jsInfo;
					jsInfo["ret"] = 0;
					jsInfo["clubID"] = getClub()->getClubID();
					jsInfo["rooms"] = jsRooms;
					sendMsgToClient(jsInfo, MSG_CLUB_APPLY_ROOM_INFO, nSenderID);*/
				}
			}

		});
	}
	else if (vLeagues.size() == 0) {
		Json::Value jsRet;
		jsRet["ret"] = 0;
		jsRet["rooms"] = jsRoomInfo;
		getClub()->getClubMgr()->getSvrApp()->responeAsyncRequest(nSenderPort, nReqSerial, nSenderID, jsRet, getClub()->getClubID());

		/*Json::Value jsInfo;
		jsInfo["ret"] = 0;
		jsInfo["clubID"] = getClub()->getClubID();
		jsInfo["rooms"] = jsRoomInfo;
		sendMsgToClient(jsInfo, MSG_CLUB_APPLY_ROOM_INFO, nSenderID);*/
	}
}

bool CClubGameData::isClubCreateThisRoom(uint32_t nRoomID) {
	return std::find_if(m_vCreatedRooms.begin(), m_vCreatedRooms.end(), [nRoomID](stRoomEntry& str) {
		return str.nRoomID == nRoomID;
	}) != m_vCreatedRooms.end();
}