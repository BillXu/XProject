#include "DDZRoom.h"
#include "DDZPlayer.h"
#include "DDZRoomStateWaitReady.h"
#include "DDZRoomStatePlayerChu.h"
#include "DDZRoomStateStartGame.h"
#include "DDZRoomStateRobotBanker.h"
#include "DDZRoomStateGameEnd.h"
#include "DDZPlayer.h"
#include "JJDDZRoomStateChaoZhuang.h"
#include "JJDDZRoomStateTuiLaChuai.h"
#include "JJDDZRoomStateWaitReadyChaoZhuangMode.h"
bool DDZRoom::init(IGameRoomManager* pRoomMgr, uint32_t nSeialNum, uint32_t nRoomID, uint16_t nSeatCnt, Json::Value& vJsOpts)
{
	GameRoom::init(pRoomMgr, nSeialNum, nRoomID, nSeatCnt, vJsOpts);
	m_nFirstRobotBankerIdx = rand() % getSeatCnt();
	m_vDiPai.clear();
	m_nBankerIdx = 0;
	m_nBankerTimes = 0;
	m_nBombCnt = 0;

	IGameRoomState* pState = nullptr;
	if (vJsOpts["isChaoZhuang"].isNull() == false && vJsOpts["isChaoZhuang"].asUInt() == 1)
	{
		pState = new JJDDZRoomStateWaitReadyChaoZhuangMode();
		addRoomState(pState);
		setInitState(pState);

		pState = new JJDDZRoomStateChaoZhuang();
		addRoomState(pState);
	}
	else
	{
		pState = new DDZRoomStateWaitReady();
		addRoomState(pState);
		setInitState(pState);
	}
	
	pState = new DDZRoomStatePlayerChu();
	addRoomState(pState);
	pState = new DDZRoomStateStartGame();
	addRoomState(pState);
	pState = new DDZRoomStateRobotBanker();
	addRoomState(pState);
	pState = new DDZRoomStateGameEnd();
	addRoomState(pState);
	pState = new JJDDZRoomStateTiLaChuai();
	addRoomState(pState);
	return true;
}

IGamePlayer* DDZRoom::createGamePlayer()
{
	return new DDZPlayer();
}

void DDZRoom::packRoomInfo(Json::Value& jsRoomInfo)
{
	GameRoom::packRoomInfo(jsRoomInfo);
	Json::Value jsDiPai;
	for (auto& ref : m_vDiPai)
	{
		jsDiPai[jsDiPai.size()] = ref;
	}

	if (jsDiPai.size() > 0)
	{
		jsRoomInfo["diPai"] = jsDiPai;
	}
	jsRoomInfo["dzIdx"] = getBankerIdx();
	jsRoomInfo["bombCnt"] = getBombCount();
	jsRoomInfo["bottom"] = getBankTimes();
}

void DDZRoom::visitPlayerInfo(IGamePlayer* pPlayer, Json::Value& jsPlayerInfo, uint32_t nVisitorSessionID)
{
	GameRoom::visitPlayerInfo(pPlayer,jsPlayerInfo,nVisitorSessionID);
	bool isStateShowCards = eRoomState_DDZ_Chu == getCurState()->getStateID() || eRoomState_JJ_DDZ_Ti_La_Chuai == getCurState()->getStateID() || eRoomState_RobotBanker == getCurState()->getStateID();
	auto p = (DDZPlayer*)pPlayer;
	if ( isStateShowCards )
	{
		jsPlayerInfo["holdCardCnt"] = p->getPlayerCard()->getHoldCardCount();
		if ( nVisitorSessionID == pPlayer->getSessionID())
		{
			Json::Value jsHoldCards;
			p->getPlayerCard()->holdCardToJson(jsHoldCards);
			jsPlayerInfo["holdCards"] = jsHoldCards;
		}
	}

	if (p->isChaoZhuang())
	{
		jsPlayerInfo["nJiaBei"] = 1;
	}

	if (p->isTiLaChuai())
	{
		jsPlayerInfo["isTiLaChuai"] = 1;
	}
}

uint8_t DDZRoom::getRoomType()
{
	if (m_jsOpts["gameType"].isNull())
	{
		LOGFMTE("do not have game type key ");
		return eGame_Max;
	}
	return m_jsOpts["gameType"].asUInt();
}

void DDZRoom::onStartGame()
{
	GameRoom::onStartGame();
	m_vDiPai.clear();
	m_nBankerIdx = 0;
	m_nBankerTimes = 0;
	m_nBombCnt = 0;

	// distribute card 
	auto nSeatCnt = getSeatCnt();
	for (uint8_t nIdx = 0; nIdx < nSeatCnt; ++nIdx)
	{
		auto p = (DDZPlayer*)getPlayerByIdx(nIdx);
		if (p == nullptr || (p->haveState(eRoomPeer_CanAct) == false))
		{
			continue;
		}

		uint8_t nCardCnt = 17;
		while (nCardCnt--)
		{
			p->getPlayerCard()->addHoldCard(getPoker()->distributeOneCard());
		}
	}

	// send start game msg
	Json::Value jsMsg;
	for (uint8_t nIdx = 0; nIdx < nSeatCnt; ++nIdx)
	{
		auto p = (DDZPlayer*)getPlayerByIdx(nIdx);
		if (p == nullptr || (p->haveState(eRoomPeer_CanAct) == false))
		{
			continue;
		}

		Json::Value jsVHoldCard;
		if (p->haveState(eRoomPeer_CanAct))
		{
			p->getPlayerCard()->holdCardToJson(jsVHoldCard);
		}
		jsMsg["vSelfCard"] = jsVHoldCard;
		sendMsgToPlayer(jsMsg, MSG_ROOM_DDZ_START_GAME, p->getSessionID());
	}

	// add replay frame 
	Json::Value jsFrame;
	for (uint8_t nIdx = 0; nIdx < nSeatCnt; ++nIdx)
	{
		auto p = (DDZPlayer*)getPlayerByIdx(nIdx);
		if (p == nullptr || (p->haveState(eRoomPeer_CanAct) == false))
		{
			continue;
		}

		Json::Value jsVHoldCard;
		if (p->haveState(eRoomPeer_CanAct))
		{
			p->getPlayerCard()->holdCardToJson(jsVHoldCard);
		}

		Json::Value jsPlayers;
		jsPlayers["idx"] = nIdx;
		jsPlayers["uid"] = p->getUserUID();
		jsPlayers["cards"] = jsVHoldCard;
		jsFrame[jsFrame.size()] = jsPlayers;
	}
	addReplayFrame(DDZ_Frame_StartGame, jsFrame);
}

void DDZRoom::onGameEnd()
{
	GameRoom::onGameEnd();
}

bool DDZRoom::canStartGame()
{
	if (GameRoom::canStartGame() == false)
	{
		return false;
	}

	uint8_t nReadyCnt = 0;
	for (uint16_t nIdx = 0; nIdx < getSeatCnt(); ++nIdx)
	{
		auto p = getPlayerByIdx(nIdx);
		if (nullptr == p)
		{
			continue;
		}

		if (p->haveState(eRoomPeer_Ready) == false)
		{
			return false;
		}
		++nReadyCnt;
	}
	return nReadyCnt >= getSeatCnt();
}

IPoker* DDZRoom::getPoker()
{
	return &m_tPoker;
}

bool DDZRoom::onMsg(Json::Value& jsmsg, uint16_t nMsgType, eMsgPort eSenderPort, uint32_t nSessionID)
{
	if (GameRoom::onMsg(jsmsg, nMsgType, eSenderPort, nSessionID))
	{
		return true;
	}

	if (MSG_DDZ_PLAYER_UPDATE_TUO_GUAN == nMsgType)
	{
		auto pPlayer = getPlayerBySessionID(nSessionID);
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
		} while (0);

		if (nRet)
		{
			jsmsg["ret"] = nRet;
			sendMsgToPlayer(jsmsg, nMsgType, nSessionID);
			return true;
		}

		getCurState()->checkTuoGuan();
		jsmsg["idx"] = pPlayer->getIdx();
		sendRoomMsg(jsmsg, MSG_DDZ_ROOM_UPDATE_TUO_GUAN);
		return true;
	}

	return false;
}

uint8_t DDZRoom::getFirstRobotBankerIdx()
{
	return m_nFirstRobotBankerIdx;
}

uint8_t DDZRoom::moveFirstRobotBankerIdx()
{
	 return m_nFirstRobotBankerIdx = (++m_nFirstRobotBankerIdx) % getSeatCnt();
}

void DDZRoom::setNewBankerInfo(uint8_t nBankerIdx, uint8_t nBankerTimes, std::vector<uint8_t>& vDiPai)
{
	m_nBankerIdx = nBankerIdx;
	m_nBankerTimes = nBankerTimes;
	m_vDiPai = vDiPai;
}

uint8_t DDZRoom::getBankTimes()
{
	return m_nBankerTimes;
}

uint8_t DDZRoom::getBombCount()
{
	return m_nBombCnt;
}

void DDZRoom::increaseBombCount()
{
	++m_nBombCnt;
}

uint32_t DDZRoom::fengDing()
{
	if (m_jsOpts["maxBet"].isNull() == false && m_jsOpts["maxBet"].isUInt() == false )
	{
		return 100000; // not limit 
	}
	return m_jsOpts["maxBet"].asUInt();
}

int32_t DDZRoom::getDeskFee()
{
	return m_jsOpts["deskFee"].asInt();
}