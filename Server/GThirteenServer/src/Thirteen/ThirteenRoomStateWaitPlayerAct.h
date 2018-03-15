#pragma once
#include "IGameRoomState.h"
#include "ISeverApp.h"
#include "AsyncRequestQuene.h"
#include "RoomManager.h"
#define Show_Cards_Add_Time 30
#define Reput_Cards_Consume_Diamond 20
#define Delay_Put_Cards_Consume_Diamond 20

class ThirteenRoomStateWaitPlayerAct
	:public IGameRoomState
{
public:
	uint32_t getStateID() { return eRoomState_WaitPlayerAct; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData) override
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		m_bWaitingOtherServer = 0;
		auto pRoom = (ThirteenRoom*)getRoom();
		pRoom->onWaitAct();
		setStateDuringTime(pRoom->isWaitPlayerActForever() ? 100000000 : pRoom->getPutCardsTime());
		//assert(0 && "invalid argument");
	}

	void onStateTimeUp() override
	{
		auto pRoom = (ThirteenRoom*)getRoom();
		if (pRoom->onAutoDoPlayerAct()) {
			//pRoom->goToState(eRoomState_GameEnd);
		}
		else {
			LOGFMTE("Big error!!! room is in error state can not auto end, id = %u", pRoom->getRoomID());
			setStateDuringTime(eTime_WaitPlayerAct);
		}
	}

	void update(float fDeta)override
	{
		IGameRoomState::update(fDeta);

		if (isWaiting()) {
			return;
		}

		auto pRoom = (ThirteenRoom*)getRoom();
		if (pRoom->isGameOver())
		{
			pRoom->goToState(eRoomState_GameEnd);
		}
	}

	bool onMsg(Json::Value& jsmsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID) override
	{
		if (MSG_ROOM_THIRTEEN_REPUT_CARDS == nMsgType) {
			auto pRoom = (ThirteenRoom*)getRoom();
			auto pPlayer = (ThirteenPlayer*)pRoom->getPlayerBySessionID(nSessionID);
			if (pPlayer == nullptr)
			{
				Json::Value jsRet;
				jsRet["ret"] = 1;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("you are not in this room how to apply reput cards? session id = %u", nSessionID);
				return true;
			}

			if (pRoom->isPlayerCanAct(pPlayer->getIdx()) == false) {
				Json::Value jsRet;
				jsRet["ret"] = 2;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("you are not in this game how to apply reput cards? session id = %u", nSessionID);
				return true;
			}

			if (pPlayer->hasDetermined() == false) {
				Json::Value jsRet;
				jsRet["ret"] = 3;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("you have not put cards how to apply reput cards? session id = %u", nSessionID);
				return true;
			}

			m_bWaitingOtherServer++;
			auto pApp = pRoom->getRoomMgr()->getSvrApp();
			Json::Value jsReq;
			jsReq["targetUID"] = pPlayer->getUserUID();
			jsReq["diamond"] = Reput_Cards_Consume_Diamond;
			jsReq["roomID"] = pRoom->getRoomID();
			pApp->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, pPlayer->getUserUID(), eAsync_thirteen_reput_check_Diamond, jsReq, [pApp, pPlayer, pRoom, nMsgType, this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
			{
				if (m_bWaitingOtherServer) {
					m_bWaitingOtherServer--;
				}

				Json::Value jsRet;
				if (isTimeOut)
				{
					LOGFMTE(" request apply reput cards time out uid = %u , can not reput cards", pPlayer->getUserUID());
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

					pPlayer->clearDeterMined();
					auto pCard = (ThirteenPeerCard*)pPlayer->getPlayerCard();
					pCard->reSetDao();
					setStateDuringTime(getStateDuring() + Show_Cards_Add_Time);

					Json::Value /*jsRet,*/ jsMsgRoom, jsTime;
					/*jsRet["ret"] = 0;
					pRoom->sendMsgToPlayer(jsRet, nMsgType, pPlayer->getSessionID());*/

					jsMsgRoom["idx"] = pPlayer->getIdx();
					jsMsgRoom["state"] = 0;
					pRoom->sendRoomMsg(jsMsgRoom, MSG_ROOM_THIRTEEN_GAME_PUT_CARDS_UPDATE);

					jsTime["idx"] = pPlayer->getIdx();
					jsTime["time"] = (int32_t)getStateDuring();
					pRoom->sendRoomMsg(jsTime, MSG_ROOM_THIRTEEN_UPDATE_CARDS_PUT_TIME);

					Json::Value jsConsumDiamond;
					jsConsumDiamond["playerUID"] = pPlayer->getUserUID();
					jsConsumDiamond["diamond"] = Reput_Cards_Consume_Diamond;
					jsConsumDiamond["roomID"] = getRoom()->getRoomID();
					jsConsumDiamond["reason"] = 1;
					pApp->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, pPlayer->getUserUID(), eAsync_Consume_Diamond, jsConsumDiamond);
					LOGFMTD("user uid = %u reput cards do comuse diamond = %u room id = %u", pPlayer->getUserUID(), Reput_Cards_Consume_Diamond, getRoom()->getRoomID());

				} while (0);

				jsRet["ret"] = nRet;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, pPlayer->getSessionID());
			});
			return true;
		}

		if (MSG_ROOM_THIRTEEN_GAME_DELAY_PUT == nMsgType) {
			auto pRoom = (ThirteenRoom*)getRoom();
			auto pPlayer = pRoom->getPlayerBySessionID(nSessionID);
			Json::Value jsRet;
			if (pPlayer == nullptr) {
				jsRet["ret"] = 1;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("you are not in this room how to delay put cards time? session id = %u", nSessionID);
				return true;
			}

			m_bWaitingOtherServer++;
			auto pApp = pRoom->getRoomMgr()->getSvrApp();
			Json::Value jsReq;
			jsReq["targetUID"] = pPlayer->getUserUID();
			jsReq["diamond"] = Reput_Cards_Consume_Diamond;
			jsReq["roomID"] = pRoom->getRoomID();
			pApp->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, pPlayer->getUserUID(), eAsync_thirteen_delay_check_Diamond, jsReq, [pApp, pPlayer, pRoom, nMsgType, this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
			{
				if (m_bWaitingOtherServer) {
					m_bWaitingOtherServer--;
				}

				Json::Value jsRet;
				if (isTimeOut)
				{
					LOGFMTE(" request apply delay put cards time out uid = %u , can not delay put cards", pPlayer->getUserUID());
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

					setStateDuringTime(getStateDuring() + Show_Cards_Add_Time);
					Json::Value jsMsg;
					jsMsg["idx"] = pPlayer->getIdx();
					jsMsg["time"] = (int32_t)getStateDuring();
					pRoom->sendRoomMsg(jsMsg, MSG_ROOM_THIRTEEN_UPDATE_CARDS_PUT_TIME);

					Json::Value jsConsumDiamond;
					jsConsumDiamond["playerUID"] = pPlayer->getUserUID();
					jsConsumDiamond["diamond"] = Reput_Cards_Consume_Diamond;
					jsConsumDiamond["roomID"] = getRoom()->getRoomID();
					jsConsumDiamond["reason"] = 1;
					pApp->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, pPlayer->getUserUID(), eAsync_Consume_Diamond, jsConsumDiamond);
					LOGFMTD("user uid = %u delay put cards do comuse diamond = %u room id = %u", pPlayer->getUserUID(), Reput_Cards_Consume_Diamond, getRoom()->getRoomID());

				} while (0);

				jsRet["ret"] = nRet;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, pPlayer->getSessionID());
			});



			/*jsRet["ret"] = 0;
			setStateDuringTime(getStateDuring() + Show_Cards_Add_Time);
			pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			Json::Value jsMsg;
			jsMsg["idx"] = pPlayer->getIdx();
			jsMsg["time"] = (int32_t)getStateDuring();
			pRoom->sendRoomMsg(jsMsg, MSG_ROOM_THIRTEEN_UPDATE_CARDS_PUT_TIME);*/
			return true;
		}

		if (MSG_ROOM_THIRTEEN_GAME_SHOW_CARDS == nMsgType) {
			auto pRoom = (ThirteenRoom*)getRoom();
			auto pPlayer = pRoom->getPlayerBySessionID(nSessionID);
			Json::Value jsRet;
			if (pPlayer == nullptr)
			{
				jsRet["ret"] = 1;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("you are not in this room how to show cards? session id = %u", nSessionID);
				return true;
			}

			if (pRoom->isCanMingPai() == false) {
				jsRet["ret"] = 3;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				LOGFMTE("this room can not show cards? room id = %u, session id = %u", pRoom->getRoomID(), nSessionID);
				return true;
			}

			m_bWaitingOtherServer++;
			auto pApp = pRoom->getRoomMgr()->getSvrApp();
			Json::Value jsReq;
			jsReq["targetUID"] = pPlayer->getUserUID();
			jsReq["baseScore"] = pRoom->getBaseScore();
			jsReq["roomID"] = pRoom->getRoomID();
			pApp->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, pPlayer->getUserUID(), eAsync_player_apply_Show_Cards, jsReq, [pApp, pPlayer, pRoom, nMsgType, this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
			{
				if (m_bWaitingOtherServer) {
					m_bWaitingOtherServer--;
				}

				Json::Value jsRet;
				if (isTimeOut)
				{
					LOGFMTE(" request apply show cards time out uid = %u , can not show cards", pPlayer->getUserUID());
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

					if (pRoom->onPlayerShowCards(pPlayer->getIdx())) {
						setStateDuringTime(getStateDuring() + Show_Cards_Add_Time);
						//jsRet["ret"] = 0;
						//jsRet["time"] = (int32_t)getStateDuring();
						//pRoom->sendMsgToPlayer(jsRet, nMsgType, pPlayer->getSessionID());
						Json::Value jsMsg, jsCards;
						jsMsg["idx"] = pPlayer->getIdx();
						((ThirteenPlayer*)pPlayer)->getPlayerCard()->groupCardToJson(jsCards);
						jsMsg["cards"] = jsCards;
						jsMsg["waitTime"] = (int32_t)getStateDuring();
						pRoom->sendRoomMsg(jsMsg, MSG_ROOM_THIRTEEN_SHOW_CARDS_UPDATE);
						Json::Value jsReq_1;
						jsReq_1["targetUID"] = pPlayer->getUserUID();
						jsReq_1["baseScore"] = pRoom->getBaseScore();
						jsReq_1["roomID"] = pRoom->getRoomID();
						pApp->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, pPlayer->getUserUID(), eAsync_player_do_Show_Cards, jsReq_1);
						//LOGFMTE("show cards success! session id = %u", nSessionID);
					}
					else {
						LOGFMTE("show cards failed? user id = %u", pPlayer->getUserUID());
						nRet = 2;
						break;
					}

				} while (0);

				jsRet["ret"] = nRet;
				pRoom->sendMsgToPlayer(jsRet, nMsgType, pPlayer->getSessionID());
			});
			return true;
		}

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
				if (jsmsg["showCards"].isNull() == false && jsmsg["showCards"].isUInt() && jsmsg["showCards"].asUInt()) {
					if (pRoom->isCanMingPai() == false) {
						jsRet["ret"] = 3;
						pRoom->sendMsgToPlayer(jsRet, MSG_ROOM_THIRTEEN_GAME_SHOW_CARDS, nSessionID);
						LOGFMTE("this room can not show cards? room id = %u, session id = %u", pRoom->getRoomID(), nSessionID);
						return true;
					}

					m_bWaitingOtherServer++;
					auto pApp = pRoom->getRoomMgr()->getSvrApp();
					Json::Value jsReq;
					jsReq["targetUID"] = pPlayer->getUserUID();
					jsReq["baseScore"] = pRoom->getBaseScore();
					jsReq["roomID"] = pRoom->getRoomID();
					pApp->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, pPlayer->getUserUID(), eAsync_player_apply_Show_Cards, jsReq, [pApp, pPlayer, pRoom, nMsgType, this](uint16_t nReqType, const Json::Value& retContent, Json::Value& jsUserData, bool isTimeOut)
					{
						if (m_bWaitingOtherServer) {
							m_bWaitingOtherServer--;
						}

						Json::Value jsRet;
						if (isTimeOut)
						{
							LOGFMTE(" request apply show cards time out uid = %u , can not show cards", pPlayer->getUserUID());
							jsRet["ret"] = 7;
							pRoom->sendMsgToPlayer(jsRet, MSG_ROOM_THIRTEEN_GAME_SHOW_CARDS, pPlayer->getSessionID());
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

							if (pRoom->onPlayerShowCards(pPlayer->getIdx())) {
								setStateDuringTime(getStateDuring() + Show_Cards_Add_Time);
								//jsRet["ret"] = 0;
								//jsRet["time"] = (int32_t)getStateDuring();
								//pRoom->sendMsgToPlayer(jsRet, nMsgType, pPlayer->getSessionID());
								Json::Value jsMsg, jsCards;
								jsMsg["idx"] = pPlayer->getIdx();
								((ThirteenPlayer*)pPlayer)->getPlayerCard()->groupCardToJson(jsCards);
								jsMsg["cards"] = jsCards;
								jsMsg["waitTime"] = (int32_t)getStateDuring();
								pRoom->sendRoomMsg(jsMsg, MSG_ROOM_THIRTEEN_SHOW_CARDS_UPDATE);
								Json::Value jsReq_1;
								jsReq_1["targetUID"] = pPlayer->getUserUID();
								jsReq_1["baseScore"] = pRoom->getBaseScore();
								jsReq_1["roomID"] = pRoom->getRoomID();
								pApp->getAsynReqQueue()->pushAsyncRequest(ID_MSG_PORT_DATA, pPlayer->getUserUID(), eAsync_player_do_Show_Cards, jsReq_1);
								//LOGFMTE("show cards success! session id = %u", nSessionID);
							}
							else {
								LOGFMTE("show cards failed? user id = %u", pPlayer->getUserUID());
								nRet = 2;
								break;
							}

						} while (0);

						jsRet["ret"] = nRet;
						pRoom->sendMsgToPlayer(jsRet, MSG_ROOM_THIRTEEN_GAME_SHOW_CARDS, pPlayer->getSessionID());
					});
				}
				return true;
			}
			jsRet["ret"] = 3;
			pRoom->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
			LOGFMTE("put cards error? session id = %u", nSessionID);
			return true;
		}

		return false;
	}

	bool isWaiting()override {
		return m_bWaitingOtherServer;
	}

protected:
	uint32_t m_bWaitingOtherServer;
};