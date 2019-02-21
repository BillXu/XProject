#pragma once
#include "IGameRoomState.h"
class DDZRoomStateDouble
	:public IGameRoomState
{
public:
	uint32_t getStateID()override {
		return eRoomState_DDZ_Double;
	}

	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		if (onWaitDouble()) {
			//setStateDuringTime(eTime_WaitChoseExchangeCard);
			setStateDuringTime(5);
		}
		else {
			setStateDuringTime(0);
		}
	}

	void update(float fDeta)override {
		if (getWaitTime() > 15.0f) {
			uint8_t nSeatCnt = getRoom()->getSeatCnt();
			for (uint8_t idx = 0; idx < nSeatCnt; idx++) {
				auto pPlayer = (DDZPlayer*)getRoom()->getPlayerByIdx(idx);
				if (pPlayer->isDoubleDone()) {
					continue;
				}
				pPlayer->addExtraTime(fDeta);
			}
		}
		IGameRoomState::update(fDeta);
	}

	void onStateTimeUp()override
	{
		uint8_t nSeatCnt = getRoom()->getSeatCnt();
		for (uint8_t idx = 0; idx < nSeatCnt; idx++) {
			auto pPlayer = (DDZPlayer*)getRoom()->getPlayerByIdx(idx);
			if (pPlayer->isDoubleDone()) {
				continue;
			}
			Json::Value jsMsg;
			jsMsg["ret"] = 0;
			jsMsg["idx"] = pPlayer->getIdx();
			jsMsg["double"] = 0;
			pPlayer->signDouble();
			getRoom()->sendRoomMsg(jsMsg, MSG_DDZ_PLAYER_DOUBLE);
		}
		getRoom()->goToState(eRoomState_DDZ_Chu);
	}

	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)
	{
		if (MSG_DDZ_PLAYER_DOUBLE == nMsgType) {
			Json::Value jsMsg;
			auto pRoom = (DDZRoom*)getRoom();
			auto pPlayer = (DDZPlayer*)pRoom->getPlayerBySessionID(nSessionID);
			if (pPlayer == nullptr) {
				jsMsg["ret"] = 1;
				pRoom->sendMsgToPlayer(jsMsg, nMsgType, nSessionID);
				return true;
			}

			if (pRoom->isCanDouble() == false) {
				jsMsg["ret"] = 2;
				pRoom->sendMsgToPlayer(jsMsg, nMsgType, nSessionID);
				return true;
			}

			if (pPlayer->isDoubleDone()) {
				jsMsg["ret"] = 3;
				pRoom->sendMsgToPlayer(jsMsg, nMsgType, nSessionID);
				return true;
			}

			uint8_t nDouble = prealMsg["double"].asUInt();
			jsMsg["ret"] = 0;
			jsMsg["idx"] = pPlayer->getIdx();
			jsMsg["double"] = nDouble;
			pPlayer->signDouble(nDouble);
			pRoom->sendRoomMsg(jsMsg, nMsgType);

			// add frame ;
			Json::Value jsFrame;
			jsFrame["idx"] = pPlayer->getIdx();
			jsFrame["double"] = nDouble;
			pRoom->addReplayFrame(DDZ_Frame_Double, jsFrame);

			if (isDoubleOver())
			{
				setStateDuringTime(0);
			}
			return true;
		}

		return false;
	}

protected:
	bool onWaitDouble() {
		auto pRoom = (DDZRoom*)getRoom();
		if (pRoom->isCanDouble()) {
			Json::Value jsMsg;
			jsMsg["state"] = 1;
			pRoom->sendRoomMsg(jsMsg, MSG_DDZ_PLAYER_DOUBLE);
			return true;
		}
		return false;
	}

	bool isDoubleOver() {
		auto pRoom = (DDZRoom*)getRoom();
		if (pRoom->isCanDouble()) {
			for (uint8_t idx = 0; idx < pRoom->getSeatCnt(); idx++) {
				auto pPlayer = (DDZPlayer*)pRoom->getPlayerByIdx(idx);
				if (pPlayer->isDoubleDone()) {
					continue;
				}
				return false;
			}
		}
		return true;
	}
};