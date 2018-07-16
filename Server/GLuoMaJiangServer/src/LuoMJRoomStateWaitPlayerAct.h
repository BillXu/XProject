#pragma once
#include "MJRoomStateWaitPlayerAct.h"
#include "LuoMJRoom.h"
class LuoMJRoomStateWaitPlayerAct
	:public MJRoomStateWaitPlayerAct
{
public:
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)override
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		if (jsTranData["idx"].isNull() == false && jsTranData["idx"].isUInt())
		{
			m_nIdx = jsTranData["idx"].asUInt();
			auto pRoom = (LuoMJRoom*)getRoom();
			pRoom->onWaitPlayerAct(m_nIdx, m_isCanPass);
			if (m_isCanPass) {
				setStateDuringTime(pRoom->isWaitPlayerActForever() ? 100000000 : eTime_WaitPlayerAct);
			}
			else {
				auto pPlayer = pRoom->getPlayerByIdx(m_nIdx);
				if (pPlayer) {
					if (pPlayer->haveState(eRoomPeer_AlreadyHu)) {
						setStateDuringTime(0);
					}
					else {
						setStateDuringTime(pRoom->isWaitPlayerActForever() ? 100000000 : eTime_WaitPlayerAct);
					}
				}
				else {
					Assert(0, "invalid argument");
					setStateDuringTime(0);
				}
			}
			return;
		}
		Assert(0, "invalid argument");
	}

	void onStateTimeUp()override
	{
		if (m_isCanPass)
		{
			m_isCanPass = false;
			auto pRoom = (LuoMJRoom*)getRoom();
			auto pPlayer = pRoom->getPlayerByIdx(m_nIdx);
			if (pPlayer) {
				if (pPlayer->haveState(eRoomPeer_AlreadyHu)) {
					setStateDuringTime(0);
				}
				else {
					setStateDuringTime(pRoom->isWaitPlayerActForever() ? 100000000 : eTime_WaitPlayerAct);
				}
			}
			else {
				Assert(0, "invalid argument");
				setStateDuringTime(0);
			}
			return;
		}
		auto nCard = ((IMJRoom*)getRoom())->getAutoChuCardWhenWaitActTimeout(m_nIdx);
		LOGFMTE("wait time out , auto chu card = %u idx = %u", nCard, m_nIdx);
		Json::Value jsTran;
		jsTran["idx"] = m_nIdx;
		jsTran["act"] = eMJAct_Chu;
		jsTran["card"] = nCard;
		getRoom()->goToState(eRoomState_DoPlayerAct, &jsTran);
	}

	bool onMsg(Json::Value& prealMsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)override {
		if (MSG_PLAYER_ACT == nMsgType)
		{
			auto pPlayer = getRoom()->getPlayerBySessionID(nSessionID);

			if (pPlayer == nullptr)
			{
				LOGFMTE("you are not in room  why req act list");
				return false;
			}

			auto actType = prealMsg["actType"].asUInt();
			if (eMJAct_Pass == actType)
			{
				if (pPlayer->haveState(eRoomPeer_AlreadyHu)) {
					setStateDuringTime(0);
				}
				else {
					auto pRoom = (LuoMJRoom*)getRoom();
					setStateDuringTime(pRoom->isWaitPlayerActForever() ? 100000000 : eTime_WaitPlayerAct);
				}
				return true;
			}
		}

		return MJRoomStateWaitPlayerAct::onMsg(prealMsg, nMsgType, eSenderPort, nSessionID);
	}
};