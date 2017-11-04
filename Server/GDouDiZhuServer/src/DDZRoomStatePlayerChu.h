#pragma once
#include "IGameRoomState.h"
#include "DDZRoom.h"
#include "DDZPlayer.h"
#include "DouDiZhuDefine.h"
#include "DouDiZhuCardTypeChecker.h"
#define TIME_DELAY_ENTER_GAME_OVER 0.5
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

		setStateDuringTime(99999999);
	}

	bool onMsg(Json::Value& jsmsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)
	{
		if ( MSG_DDZ_PLAYER_CHU != nMsgType && MSG_DDZ_PLAYER_SHOW_CARDS != nMsgType )
		{
			return false;
		}

		auto pRoom = (DDZRoom*)getRoom();
		auto pPlayer = (DDZPlayer*)pRoom->getPlayerBySessionID(nSessionID);

		if ( MSG_DDZ_PLAYER_SHOW_CARDS != nMsgType)
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
				jsRet[""] = nRet;
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
			// tell other players ;
			jsmsg["idx"] = pPlayer->getIdx();
			getRoom()->sendRoomMsg(jsmsg, MSG_DDZ_ROOM_CHU); 

			// next player do act ;
			infomNextPlayerAct();
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
			bool isValid = m_tCurMaxChuPai.tChuPaiType == nType && nWeight >= m_tCurMaxChuPai.nWeight;
			isVlaid = isValid || ( (nType > DDZ_Common) && ( nType > m_tCurMaxChuPai.tChuPaiType) );
			if (!isValid)
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

		// send msg tell player act ;
		Json::Value jsMsgBack;
		jsMsgBack["idx"] = m_nWaitChuPlayerIdx;
		getRoom()->sendRoomMsg(jsMsgBack, MSG_DDZ_ROOM_WAIT_CHU);
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
	uint8_t m_nWaitChuPlayerIdx;
	stChuPaiInfo m_tCurMaxChuPai;
};