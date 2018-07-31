#pragma once
#include "IGameRoomState.h"
#include "DDZRoom.h"
#include "DDZPlayer.h"
#include "DouDiZhuDefine.h"
#include "DouDiZhuCardTypeChecker.h"
#define TIME_DELAY_ENTER_GAME_OVER 0.5
#define TIME_TUOGUAN_DELAY_ACT 2
class DDZRoomStatePlayerChu
	:public IGameRoomState
{
public:
	struct stChuPaiInfo
	{
		std::vector<uint8_t> vCards;
		DDZ_Type tChuPaiType;
		uint32_t nWeight;
		uint8_t nPlayerIdx;
		stChuPaiInfo() { clear(); }
		void clear() 
		{
			vCards.clear();
			tChuPaiType = DDZ_Max;
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
		auto pRoom = (DDZRoom*)getRoom();
		m_nWaitChuPlayerIdx = pRoom->getBankerIdx();
		m_tCurMaxChuPai.clear();
		// send msg tell player act ;
		Json::Value jsMsgBack;
		jsMsgBack["idx"] = m_nWaitChuPlayerIdx;
		getRoom()->sendRoomMsg(jsMsgBack, MSG_DDZ_ROOM_WAIT_CHU);

		// add frame 
		Json::Value jsFrame;
		jsFrame["idx"] = m_nWaitChuPlayerIdx;
		getRoom()->addReplayFrame(DDZ_Frame_WaitChu, jsFrame);
		// check tuoGuan 
		checkTuoGuan();
		setStateDuringTime(eTime_WaitPlayerAct);
	}

	bool onMsg(Json::Value& jsmsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)
	{
		if ( MSG_DDZ_PLAYER_CHU != nMsgType && MSG_DDZ_PLAYER_SHOW_CARDS != nMsgType )
		{
			return false;
		}

		auto pRoom = (DDZRoom*)getRoom();
		auto pPlayer = (DDZPlayer*)pRoom->getPlayerBySessionID(nSessionID);

		/*if ( MSG_DDZ_PLAYER_UPDATE_TUO_GUAN == nMsgType )
		{
			uint8_t nRet = 0;
			bool isTuoGuan = false;
			do
			{
				if (pPlayer == nullptr)
				{
					nRet = 2;
					break;
				}

				if (jsmsg["isTuoGuan"].isNull() || jsmsg["isTuoGuan"].isInt() == false)
				{
					nRet = 3;
					break;
				}
				isTuoGuan = jsmsg["isTuoGuan"].asUInt() == 1;
				bool isCurTuoGuan = pPlayer->isTuoGuan();
				if (isTuoGuan == isCurTuoGuan)
				{
					nRet = 1;
					break;
				}
				pPlayer->setTuoGuanFlag(isTuoGuan);
			} while ( 0 );

			if (nRet)
			{
				jsmsg["ret"] = nRet;
				getRoom()->sendMsgToPlayer(jsmsg, nMsgType, nSessionID);
				return true;
			}

			jsmsg["idx"] = pPlayer->getIdx();
			getRoom()->sendRoomMsg(jsmsg, MSG_DDZ_ROOM_UPDATE_TUO_GUAN);
			return true;
		}*/

		if ( MSG_DDZ_PLAYER_SHOW_CARDS == nMsgType)
		{
			Json::Value jsRet;
			uint8_t nRet = 0;
			do
			{
				if (pPlayer == nullptr)
				{
					nRet = 4;
					break;
				}

				if (pRoom->getBankerIdx() != pPlayer->getIdx())
				{
					nRet = 1;
					break;
				}

				if (pPlayer->isMingPai())
				{
					nRet = 2;
					break;
				}

				if (pPlayer->getPlayerCard()->getChuedCardTimes() > 0)
				{
					nRet = 3;
					break;
				}
			} while ( 0 );

			if (nRet)
			{
				jsRet["ret"] = nRet;
				getRoom()->sendMsgToPlayer(jsRet, nMsgType, nSessionID);
				return true;
			}

			pPlayer->doMingPai();

			Json::Value jsMsg;
			jsMsg["idx"] = pPlayer->getIdx();
			Json::Value jsHold;
			pPlayer->getPlayerCard()->holdCardToJson(jsHold);
			jsMsg["cards"] = jsHold;
			getRoom()->sendRoomMsg(jsMsg, MSG_DDZ_ROOM_SHOW_CARDS);
			return true;
		}

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
			if ( m_tCurMaxChuPai.nPlayerIdx == m_nWaitChuPlayerIdx )
			{
				js["ret"] = 6;
				pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
				return true;
			}
			// tell other players ;
			jsmsg["idx"] = pPlayer->getIdx();
			getRoom()->sendRoomMsg(jsmsg, MSG_DDZ_ROOM_CHU); 
			pPlayer->getPlayerCard()->clearLastChu();
			// next player do act ;
			infomNextPlayerAct();

			// add frame ;
			Json::Value jsFrame;
			getRoom()->addReplayFrame(DDZ_Frame_DoChu, jsFrame);
			return true;
		}

		// parse value 
		auto nType = (DDZ_Type)jsmsg["type"].asUInt();
		std::vector<uint8_t> vChuCarrds;
		for (auto nCardIdx = 0; nCardIdx < jsmsg["cards"].size(); ++nCardIdx)
		{
			vChuCarrds.push_back(jsmsg["cards"][nCardIdx].asUInt());
		}

		// check card type invalid 
		uint8_t nWeight = 0;
		auto isVlaid = DDZCardTypeChecker::getInstance()->isCardTypeValid(vChuCarrds, nType, nWeight );
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
			isVlaid = isVlaid || ( (nType > DDZ_Common) && ( nType > m_tCurMaxChuPai.tChuPaiType) );
			if (!isVlaid)
			{
				js["ret"] = 2;
				pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
				LOGFMTE("card is small cann't chu room id = %u , idx = %u", getRoom()->getRoomID(), pPlayer->getIdx());
				return true;
			}
		}

		// do chu 
		if (!pPlayer->getPlayerCard()->onChuCard(vChuCarrds))
		{
			js["ret"] = 1;
			pRoom->sendMsgToPlayer(js, nMsgType, nSessionID);
			return true;
		}

		// add frame ;
		Json::Value jsFrame;
		jsFrame["cards"] = jsmsg["cards"];
		jsFrame["type"] = jsmsg["type"];
		getRoom()->addReplayFrame(DDZ_Frame_DoChu, jsFrame);
		
		// update max card ;
		m_tCurMaxChuPai.nWeight = nWeight;
		m_tCurMaxChuPai.vCards = vChuCarrds;
		m_tCurMaxChuPai.nPlayerIdx = pPlayer->getIdx();
		m_tCurMaxChuPai.tChuPaiType = nType;
		if ( DDZ_Bomb == nType || DDZ_Rokect == nType)
		{
			pRoom->increaseBombCount();
		}
		
		// tell other players ;
		jsmsg["idx"] = pPlayer->getIdx();
		getRoom()->sendRoomMsg(jsmsg,MSG_DDZ_ROOM_CHU);
		if ( pPlayer->getPlayerCard()->getHoldCardCount() == 0 ) // game over 
		{
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
		js["maxType"] = m_tCurMaxChuPai.tChuPaiType;

		Json::Value jsLastChuArray;
		for (uint8_t nIdx = 0; nIdx < getRoom()->getSeatCnt(); ++nIdx)
		{
			auto p = (DDZPlayer*)getRoom()->getPlayerByIdx(nIdx);
			if (p == nullptr)
			{
				continue;
			}

			Json::Value jsPlayer;
			jsPlayer["idx"] = nIdx;

			Json::Value jsLastChu;
			p->getPlayerCard()->lastChuToJson(jsLastChu);
			if (jsLastChu.size() > 0)
			{
				jsPlayer["chu"] = jsLastChu;
			}

			jsLastChuArray[jsLastChuArray.size()] = jsPlayer;
		}
		js["lastChu"] = jsLastChuArray;
	}
protected:
	void infomNextPlayerAct()
	{
		// inform next player ;
		auto nSeatCnt = (int16_t)getRoom()->getSeatCnt();
		m_nWaitChuPlayerIdx = ++m_nWaitChuPlayerIdx % nSeatCnt;

		while (getRoom()->getPlayerByIdx(m_nWaitChuPlayerIdx) == nullptr && nSeatCnt-- > 0)
		{
			LOGFMTE("why this player  is null idx = %u, roomid = %u", m_nWaitChuPlayerIdx,getRoom()->getRoomID() );
			m_nWaitChuPlayerIdx = ++m_nWaitChuPlayerIdx % getRoom()->getSeatCnt();  // try one more time ;
		}

		if ( m_nWaitChuPlayerIdx == m_tCurMaxChuPai.nPlayerIdx )
		{
			auto p = (DDZPlayer*)getRoom()->getPlayerByIdx(m_nWaitChuPlayerIdx);
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
		getRoom()->addReplayFrame(DDZ_Frame_WaitChu, jsFrame);
		// check tuoGuan 
		setStateDuringTime(eTime_WaitPlayerAct);
		checkTuoGuan();
	}

	void delayEnterGameOverState()
	{
		m_nWaitChuPlayerIdx = -1;
		getRoom()->goToState(eRoomState_GameEnd);
	}

	void onStateTimeUp()override
	{
		auto p = getRoom()->getPlayerByIdx(m_nWaitChuPlayerIdx);
		if ( !p )
		{
			LOGFMTE( "why wait cur player is null ? idx = %d , roomID = %u",m_nWaitChuPlayerIdx , getRoom()->getRoomID() );
			return;
		}
		Json::Value jsAutoChu;
		onMsg(jsAutoChu, MSG_DDZ_PLAYER_CHU, ID_MSG_PORT_CLIENT, p->getSessionID());
	}

	void checkTuoGuan()override
	{
		auto p = (DDZPlayer*)getRoom()->getPlayerByIdx(m_nWaitChuPlayerIdx);
		if (p && p->isTuoGuan())
		{
			// do auto chu 
			auto nCurType = m_tCurMaxChuPai.tChuPaiType;
			if (m_tCurMaxChuPai.nPlayerIdx == m_nWaitChuPlayerIdx)
			{
				nCurType = DDZ_Max;
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
			onMsg(jsAutoChu, MSG_DDZ_PLAYER_CHU, ID_MSG_PORT_CLIENT, p->getSessionID());

			/// delay act ;
			//m_tTuoGuanTimer.reset();
			//m_tTuoGuanTimer.setInterval(TIME_TUOGUAN_DELAY_ACT);
			//m_tTuoGuanTimer.setIsAutoRepeat(false);
			//m_tTuoGuanTimer.setCallBack([this, p](CTimer* t, float f)
			//{
			//	if (p->isTuoGuan() == false || p->getIdx() != m_nWaitChuPlayerIdx)
			//	{
			//		return;
			//	}

			//	// do auto chu 
			//	auto nCurType = m_tCurMaxChuPai.tChuPaiType;
			//	if (m_tCurMaxChuPai.nPlayerIdx == m_nWaitChuPlayerIdx)
			//	{
			//		nCurType = DDZ_Max;
			//	}

			//	std::vector<uint8_t> vAutChuCards;
			//	auto isChu = p->getPlayerCard()->getTuoGuanChuCards(nCurType, m_tCurMaxChuPai.vCards, vAutChuCards);
			//	Json::Value jsAutoChu;
			//	if (isChu)
			//	{
			//		Json::Value jsArray;
			//		jsAutoChu["type"] = nCurType;
			//		for (auto& ref : vAutChuCards)
			//		{
			//			jsArray[jsArray.size()] = ref;
			//		}
			//		jsAutoChu["cards"] = jsArray;
			//	}
			//	onMsg(jsAutoChu, MSG_DDZ_PLAYER_CHU, ID_MSG_PORT_CLIENT, p->getSessionID());

			//});
			//m_tTuoGuanTimer.start();
		}
	}
protected:
	uint8_t m_nWaitChuPlayerIdx;
	stChuPaiInfo m_tCurMaxChuPai;

	CTimer m_tTuoGuanTimer;
};