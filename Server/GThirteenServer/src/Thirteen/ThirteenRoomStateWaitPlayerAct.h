#pragma once
#include "IGameRoomState.h"

class ThirteenRoomStateWaitPlayerAct
	:public IGameRoomState
{
public:
	uint32_t getStateID() { return eRoomState_WaitPlayerAct; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData) override
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		auto pRoom = (ThirteenRoom*)getRoom();
		pRoom->onWaitAct();
		setStateDuringTime(pRoom->isWaitPlayerActForever() ? 100000000 : eTime_GoldenChoseAct);
		assert(0 && "invalid argument");
	}

	void onStateTimeUp() override
	{
		auto pRoom = (ThirteenRoom*)getRoom();
		if (pRoom->onAutoDoPlayerAct()) {
			pRoom->goToState(eRoomState_GameEnd);
		}
		else {
			LOGFMTE("Big error!!! room is in error state can not auto end, id = %u", pRoom->getRoomID());
			setStateDuringTime(eTime_WaitPlayerAct);
		}
	}

	bool onMsg(Json::Value& jsmsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID) override
	{
		if (MSG_ROOM_THIRTEEN_GAME_WAIT_ACT == nMsgType) {
			auto pRoom = (ThirteenRoom*)getRoom();
			auto pPlayer = pRoom->getPlayerBySessionID(nSessionID);
			Json::Value jsRet;
			if (pPlayer == nullptr)
			{
				jsRet["ret"] = 1;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("you are not in this room how to request acts list ? session id = %u", nSessionID);
				return true;
			}
			pRoom->onWaitPlayerAct(pPlayer->getIdx());
			return true;
		}

		if (MSG_ROOM_THIRTEEN_GAME_PUT_CARDS == nMsgType) {
			auto pRoom = (ThirteenRoom*)getRoom();
			auto pPlayer = (ThirteenPlayer*)pRoom->getPlayerBySessionID(nSessionID);
			Json::Value jsRet;
			if (pPlayer == nullptr)
			{
				jsRet["ret"] = 1;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("you are not in this room how to put cards ? session id = %u", nSessionID);
				return true;
			}
			if (pPlayer->hasDetermined()) {
				jsRet["ret"] = 4;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("you have already put cards ? session id = %u", nSessionID);
				return true;
			}
			if (jsmsg["cards"].isNull() || jsmsg["cards"].isArray() == false || jsmsg["cards"].size() != MAX_HOLD_CARD_COUNT) {
				jsRet["ret"] = 2;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("has wrong cards info how to put cards ? session id = %u", nSessionID);
				return true;
			}
			auto vCardsInfo = jsmsg["cards"];
			ThirteenPeerCard::VEC_CARD vCards;
			for (auto ref : vCardsInfo) {
				if (ref.isNull() || ref.isUInt() == false) {
					jsRet["ret"] = 2;
					pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
					LOGFMTE("has wrong cards info how to put cards ? session id = %u", nSessionID);
					return true;
				}
				uint8_t tCard = ref.asUInt();
				if (tCard) {
					vCards.push_back(tCard);
				}
				else {
					jsRet["ret"] = 2;
					pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
					LOGFMTE("has wrong cards info how to put cards ? session id = %u", nSessionID);
					return true;
				}
			}
			if (pRoom->onPlayerSetDao(pPlayer->getIdx(), vCards)) {
				return true;
			}
			jsRet["ret"] = 3;
			pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("put cards error? session id = %u", nSessionID);
			return true;
		}

		return false;
	}
};