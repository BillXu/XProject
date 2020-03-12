#pragma once
#include "IGameRoomState.h"
#include "GDRoom.h"
#include "GDPlayer.h"
#include "GuanDanDefine.h"
//#include "DouDiZhuCardTypeChecker.h"
#include "GDGroupCardsTypeChecker.h"
#define TIME_DELAY_ENTER_GAME_OVER 0.5
#define TIME_TUOGUAN_DELAY_ACT 2
class GDRoomStatePlayerChu
	:public IGameRoomState
{
public:
	struct stChuPaiInfo
	{
		std::vector<uint8_t> vCards;
		GD_Type tChuPaiType;
		uint32_t nWeight;
		uint8_t nPlayerIdx;
		stChuPaiInfo() { clear(); }
		void clear() 
		{
			vCards.clear();
			tChuPaiType = GD_Max;
			nWeight = 0;
			nPlayerIdx = 0;
		}
		bool isNull() { return vCards.empty(); }
	};
public:
	uint32_t getStateID() { return eRoomState_DDZ_Chu; }
	void enterState(GameRoom* pmjRoom, Json::Value& jsTranData)
	{
		IGameRoomState::enterState(pmjRoom, jsTranData);
		auto pRoom = (GDRoom*)getRoom();
		m_nWaitChuPlayerIdx = pRoom->getFirstChu();
		m_tCurMaxChuPai.clear();
		// send msg tell player act ;
		Json::Value jsMsgBack;
		jsMsgBack["idx"] = m_nWaitChuPlayerIdx;
		getRoom()->sendRoomMsg(jsMsgBack, MSG_DDZ_ROOM_WAIT_CHU);

		// add frame 
		Json::Value jsFrame;
		jsFrame["idx"] = m_nWaitChuPlayerIdx;
		getRoom()->addReplayFrame(GD_Frame_WaitChu, jsFrame);
		// check tuoGuan 
		checkTuoGuan();
		setStateDuringTime(eTime_WaitForever);
	}

	void update(float fDeta)override {
		if (getWaitTime() > 15.0f) {
			auto pPlayer = (GDPlayer*)getRoom()->getPlayerByIdx(m_nWaitChuPlayerIdx);
			pPlayer->addExtraTime(fDeta);
		}
		IGameRoomState::update(fDeta);
	}

	bool onMsg(Json::Value& jsmsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)
	{
		if ( MSG_DDZ_PLAYER_CHU != nMsgType)
		{
			return false;
		}

		auto pRoom = (GDRoom*)getRoom();
		auto pPlayer = (GDPlayer*)pRoom->getPlayerBySessionID(nSessionID);

		Json::Value js;
		if (!pPlayer)
		{
			js["ret"] = 3;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		if ( m_nWaitChuPlayerIdx != pPlayer->getIdx() )
		{
			js["ret"] = 5;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		if (jsmsg["cards"].isArray() == false || jsmsg["type"].isNull())  // player do not chu ;
		{
			// tell other players ;
			jsmsg["idx"] = pPlayer->getIdx();
			pRoom->sendRoomMsg(jsmsg, MSG_DDZ_ROOM_CHU);
			pPlayer->getPlayerCard()->clearLastChu();

			// add frame ;
			Json::Value jsFrame;
			jsFrame["idx"] = pPlayer->getIdx();
			pRoom->addReplayFrame(GD_Frame_DoChu, jsFrame);

			// next player do act ;
			infomNextPlayerAct();
			return true;
		}

		// parse value 
		auto nType = (GD_Type)jsmsg["type"].asUInt();
		std::vector<uint8_t> vChuCarrds;
		for (auto nCardIdx = 0; nCardIdx < jsmsg["cards"].size(); ++nCardIdx)
		{
			vChuCarrds.push_back(jsmsg["cards"][nCardIdx].asUInt());
		}

		// check card type invalid 
		uint32_t nWeight = 0;
		GD_Type tType = GD_None;
		//auto isVlaid = GDCardTypeChecker::getInstance()->isCardTypeValid(vChuCarrds, nType, nWeight );
		auto isVlaid = FALCardTypeChecker::getInstance()->checkCardType(vChuCarrds, nWeight, tType, nType);
		if (isVlaid == false)
		{
			js["ret"] = 2;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		// check da xiao valid ;
		if ( m_tCurMaxChuPai.isNull() == false && m_tCurMaxChuPai.nPlayerIdx != pPlayer->getIdx() )
		{
			isVlaid = m_tCurMaxChuPai.tChuPaiType == nType && nWeight >= m_tCurMaxChuPai.nWeight;
			isVlaid = isVlaid || ( (nType > eFALCardType_NotBomb) && ( nType > m_tCurMaxChuPai.tChuPaiType) );
			if (!isVlaid)
			{
				js["ret"] = 2;
				pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
				LOGFMTE("card is small cann't chu room id = %u , idx = %u", pRoom->getRoomID(), pPlayer->getIdx());
				return true;
			}
		}

		// do chu 
		if (!pPlayer->getPlayerCard()->onChuCard(vChuCarrds, nType))
		{
			js["ret"] = 1;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		// add not shuffle cards
		pRoom->addNotShuffleCards(vChuCarrds);

		// add frame ;
		Json::Value jsFrame;
		jsFrame["idx"] = pPlayer->getIdx();
		jsFrame["cards"] = jsmsg["cards"];
		jsFrame["type"] = jsmsg["type"];
		pRoom->addReplayFrame(GD_Frame_DoChu, jsFrame);
		
		// update max card ;
		m_tCurMaxChuPai.nWeight = nWeight;
		m_tCurMaxChuPai.vCards = vChuCarrds;
		m_tCurMaxChuPai.nPlayerIdx = pPlayer->getIdx();
		m_tCurMaxChuPai.tChuPaiType = nType;
		if (/* GD_Bomb == nType || GD_Rokect == nType*/nType > eFALCardType_NotBomb)
		{
			pRoom->increaseBombCount();
		}
		
		// tell other players ;
		jsmsg["idx"] = pPlayer->getIdx();
		pRoom->sendRoomMsg(jsmsg,MSG_GD_ROOM_CHU);
		if ( pPlayer->getPlayerCard()->getHoldCardCount() == 0 ) // game over 
		{
			pRoom->setFirstRotBankerIdx(m_nWaitChuPlayerIdx);
			delayEnterGameOverState();  // delay enter game over state ;
		}
		else
		{
			infomNextPlayerAct();
		}
		return true;
	}

	uint8_t getCurIdx()override { return m_nWaitChuPlayerIdx; };

	void roomInfoVisitor(Json::Value& js)override
	{
		IGameRoomState::roomInfoVisitor(js);
		js["curActIdx"] = m_nWaitChuPlayerIdx;

		Json::Value jsLastChuArray;
		for (uint8_t nIdx = 0; nIdx < getRoom()->getSeatCnt(); ++nIdx)
		{
			auto p = (GDPlayer*)getRoom()->getPlayerByIdx(nIdx);
			if (p == nullptr)
			{
				continue;
			}

			Json::Value jsPlayer;
			jsPlayer["idx"] = nIdx;
			p->getPlayerCard()->lastChuToJson(jsPlayer);
			//Json::Value jsLastChu;
			//if (p->getPlayerCard()->lastChuToJson(jsPlayer))
			//{
			//	//jsPlayer["chu"] = jsLastChu;
			//}

			jsLastChuArray[jsLastChuArray.size()] = jsPlayer;
		}
		js["lastChu"] = jsLastChuArray;
	}
protected:
	void infomNextPlayerAct()
	{
		// inform next player ;
		auto nSeatCnt = getRoom()->getSeatCnt();
		m_nWaitChuPlayerIdx = ++m_nWaitChuPlayerIdx % nSeatCnt;

		while (getRoom()->getPlayerByIdx(m_nWaitChuPlayerIdx) == nullptr && nSeatCnt-- > 0)
		{
			LOGFMTE("why this player  is null idx = %u, roomid = %u", m_nWaitChuPlayerIdx,getRoom()->getRoomID() );
			m_nWaitChuPlayerIdx = ++m_nWaitChuPlayerIdx % getRoom()->getSeatCnt();  // try one more time ;
		}

		if ( m_nWaitChuPlayerIdx == m_tCurMaxChuPai.nPlayerIdx )
		{
			auto p = (GDPlayer*)getRoom()->getPlayerByIdx(m_nWaitChuPlayerIdx);
			if (p)
			{
				p->getPlayerCard()->clearLastChu();
			}
		}
		// send msg tell player act ;
		Json::Value jsMsgBack;
		jsMsgBack["idx"] = m_nWaitChuPlayerIdx;
		getRoom()->sendRoomMsg(jsMsgBack, MSG_DDZ_ROOM_WAIT_CHU);

		// add frame 
		Json::Value jsFrame;
		jsFrame["idx"] = m_nWaitChuPlayerIdx;
		getRoom()->addReplayFrame(GD_Frame_WaitChu, jsFrame);
		// check tuoGuan 
		checkTuoGuan();
		setStateDuringTime(eTime_WaitForever);
	}

	void delayEnterGameOverState()
	{
		m_nWaitChuPlayerIdx = -1;
		setStateDuringTime(TIME_DELAY_ENTER_GAME_OVER);
	}

	void onStateTimeUp()override
	{
		getRoom()->goToState(eRoomState_GameEnd);
	}

protected:
	void checkTuoGuan()
	{
		auto p = (GDPlayer*)getRoom()->getPlayerByIdx(m_nWaitChuPlayerIdx);
		if (p && p->isTuoGuan())
		{
			m_tTuoGuanTimer.reset();
			m_tTuoGuanTimer.setInterval(TIME_TUOGUAN_DELAY_ACT);
			m_tTuoGuanTimer.setIsAutoRepeat(false);
			m_tTuoGuanTimer.setCallBack([this, p](CTimer* t, float f)
			{
				if (p->isTuoGuan() == false || p->getIdx() != m_nWaitChuPlayerIdx)
				{
					return;
				}

				// do auto chu 
				auto nCurType = m_tCurMaxChuPai.tChuPaiType;
				if (m_tCurMaxChuPai.nPlayerIdx == m_nWaitChuPlayerIdx)
				{
					nCurType = eFALCardType_Max;
				}

				std::vector<uint8_t> vAutChuCards;
				auto isChu = p->getPlayerCard()->getTuoGuanChuCards(nCurType, m_tCurMaxChuPai.vCards, vAutChuCards);
				Json::Value jsAutoChu;
				if (isChu)
				{
					Json::Value jsArray;
					jsAutoChu["type"] = nCurType;
					for (auto& ref : vAutChuCards)
					{
						jsArray[jsArray.size()] = ref;
					}
					jsAutoChu["cards"] = jsArray;
				}
				onMsg(jsAutoChu, MSG_GD_PLAYER_CHU, ID_MSG_PORT_CLIENT, p->getSessionID());

			});
			m_tTuoGuanTimer.start();
		}
	}
protected:
	uint8_t m_nWaitChuPlayerIdx;
	stChuPaiInfo m_tCurMaxChuPai;

	CTimer m_tTuoGuanTimer;
};