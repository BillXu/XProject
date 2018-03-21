#pragma once
#include "IGameRoomState.h"
class ThirteenRoomStateRobBanker
	:public IGameRoomState
{
public:
	uint32_t getStateID()override { return eRoomState_RobotBanker; }

	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)override
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		auto pRoom = (ThirteenRoom*)getRoom();
		if (pRoom->isCanRotBanker()) {
			setStateDuringTime(10);
			Json::Value jsMsg;
			getRoom()->sendRoomMsg(jsMsg, MSG_ROOM_THIRTEEN_START_ROT_BANKER);
		}
		else {
			setStateDuringTime(0);
		}
	}

	void onStateTimeUp()override
	{
		getRoom()->goToState(eRoomState_DistributeCard);
	}

	void update(float fDeta)override
	{
		IGameRoomState::update(fDeta);
		auto pRoom = (ThirteenRoom*)getRoom();
		if (pRoom->isFinishRotBanker())
		{
			pRoom->goToState(eRoomState_DistributeCard);
		}
	}

	bool onMsg(Json::Value& jsmsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID) override
	{
		if (MSG_ROOM_THIRTEEN_ROT_BANKER == nMsgType) {
			auto pRoom = (ThirteenRoom*)getRoom();
			auto pPlayer = pRoom->getPlayerBySessionID(nSessionID);
			Json::Value jsRet;
			if (pPlayer == nullptr)
			{
				jsRet["ret"] = 1;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("you are not in this room how to rot banker session id = %u", nSessionID);
				return true;
			}
			if (pRoom->isCanRotBanker() == false) {
				jsRet["ret"] = 3;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("this room can not rot banker, roomID = %u, session id = %u", pRoom->getRoomID(), nSessionID);
				return true;
			}
			uint8_t nState = 0;
			if (jsmsg["state"].isNull() == false && jsmsg["state"].isUInt()) {
				nState = jsmsg["state"].asUInt();
			}
			/*else {
				auto sState = jsmsg["state"].asCString();
				LOGFMTE("state = %s, %u", sState, jsmsg["state"].type());
			}*/

			if (nState) {
				auto pApp = pRoom->getRoomMgr()->getSvrApp();
				Json::Value jsReq;
				jsReq["targetUID"] = pPlayer->getUserUID();
				jsReq["baseScore"] = pRoom->getBaseScore();
				jsReq["roomID"] = pRoom->getRoomID();
				pApp->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, pPlayer->getUserUID(), eAsync_player_apply_Rot_Banker, jsReq, [pApp, pPlayer, pRoom, nState, nMsgType, this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
				{
					Json::Value jsRet;
					if (isTimeOut)
					{
						LOGFMTE(" request apply rot banker time out uid = %u , can not rot banker", pPlayer->getUserUID());
						jsRet["ret"] = 7;
						pRoom->sendMsgToPlayer(jsRet, nMsgType, pPlayer->getSessionID());
						return;
					}

					uint8_t nReqRet = retContent["ret"].asUInt();
					uint8_t nRet = 0;
					do {
						if (0 != nReqRet)
						{
							nRet = 4;
							break;
						}

						if (pRoom->isPlayerCanRotBanker(pPlayer->getIdx())) {
							if (pRoom->onPlayerRotBanker(pPlayer->getIdx(), nState)) {
								nRet = 0;
								Json::Value jsReq_1;
								jsReq_1["targetUID"] = pPlayer->getUserUID();
								jsReq_1["baseScore"] = pRoom->getBaseScore();
								jsReq_1["roomID"] = pRoom->getRoomID();
								pApp->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, pPlayer->getUserUID(), eAsync_player_do_Rot_Banker, jsReq_1);
							}
							else {
								nRet = 2;
							}
						}
						else {
							nRet = 5;
							LOGFMTE("player can not rot banker, roomID = %u, player id = %u", pRoom->getRoomID(), pPlayer->getUserUID());
						}

					} while (0);

					jsRet["ret"] = nRet;
					pRoom->sendMsgToPlayer(jsRet, nMsgType, pPlayer->getSessionID());
				});
			}
			else {
				if (pRoom->onPlayerRotBanker(pPlayer->getIdx(), nState)) {
					jsRet["ret"] = 0;
					pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				}
				else {
					jsRet["ret"] = 2;
					pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				}
			}
			return true;
		}
		return false;
	}
};